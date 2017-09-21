#ifndef HORN__HPP__
#define HORN__HPP__

#include "ae/ExprSimpl.hpp"

using namespace std;
using namespace boost;

namespace ufo
{
  inline bool rewriteHelperConsts(Expr& body, Expr v1, Expr v2)
  {
    if (isOpX<MPZ>(v1))
    {
      body = mk<AND>(body, mk<EQ>(v1, v2));
      return true;
    }
    //else.. TODO: simplifications like "1 + 2" and support for other sorts like Reals and Bools
    return false;
  }

  inline void rewriteHelperDupls(Expr& body, Expr _v1, Expr _v2, Expr v1, Expr v2)
  {
    if (_v1 == _v2) body = mk<AND>(body, mk<EQ>(v1, v2));
    //else.. TODO: mine more complex relationships, like "_v1 + 1 = _v2"
  }

  struct HornRuleExt
  {
    ExprVector srcVars;
    ExprVector dstVars;
    ExprVector locVars;

    Expr body;
    Expr head;

    Expr srcRelation;
    Expr dstRelation;

    bool isFact;
    bool isQuery;
    bool isInductive;

    string suffix;

    void assignVarsAndRewrite (ExprVector& _srcVars, ExprVector& invVarsSrc,
                               ExprVector& _dstVars, ExprVector& invVarsDst)
    {
      for (int i = 0; i < _srcVars.size(); i++)
      {
        srcVars.push_back(invVarsSrc[i]);

        // find constants
        if (rewriteHelperConsts(body, _srcVars[i], srcVars[i])) continue;

        body = replaceAll(body, _srcVars[i], srcVars[i]);
        for (int j = 0; j < i; j++) // find duplicates among srcVars
        {
          rewriteHelperDupls(body, _srcVars[i], _srcVars[j], srcVars[i], srcVars[j]);
        }
      }

      for (int i = 0; i < _dstVars.size(); i++)
      {
        // primed copy of var:
        Expr new_name = mkTerm<string> (lexical_cast<string>(invVarsDst[i]) + "__", body->getFactory());
        Expr var = bind::intConst(new_name);
        dstVars.push_back(var);

        // find constants
        if (rewriteHelperConsts(body, _dstVars[i], dstVars[i])) continue;

        body = replaceAll(body, _dstVars[i], dstVars[i]);
        for (int j = 0; j < i; j++) // find duplicates among dstVars
        {
          rewriteHelperDupls(body, _dstVars[i], _dstVars[j], dstVars[i], dstVars[j]);
        }
        for (int j = 0; j < _srcVars.size(); j++) // find duplicates between srcVars and dstVars
        {
          rewriteHelperDupls(body, _dstVars[i], _srcVars[j], dstVars[i], srcVars[j]);
        }
      }
    }
  };

  class CHCs
  {
    public:

    ExprFactory &m_efac;
    EZ3 &m_z3;

    Expr failDecl;
    vector<HornRuleExt> chcs;
    ExprSet decls;
    map<Expr, ExprVector> invVars;
    map<Expr, vector<int>> outgs;

    CHCs(ExprFactory &efac, EZ3 &z3) : m_efac(efac), m_z3(z3)  {};

    void preprocess (Expr term, ExprVector& srcVars, ExprVector& relations, Expr &srcRelation, ExprVector& lin)
    {
      if (isOpX<AND>(term))
      {
        for (auto it = term->args_begin(), end = term->args_end(); it != end; ++it)
        {
          preprocess(*it, srcVars, relations, srcRelation, lin);
        }
      }
      else
      {
        if (isOpX<FAPP>(term))
        {
          if (term->arity() > 0)
          {
            if (isOpX<FDECL>(term->arg(0)))
            {
              for (auto &rel: relations)
              {
                if (rel == term->arg(0))
                {
                  srcRelation = rel->arg(0);
                  for (auto it = term->args_begin()+1, end = term->args_end(); it != end; ++it)
                    srcVars.push_back(*it);
                }
              }
            }
          }
        }
        else
        {
          lin.push_back(term);
        }
      }
    }

    void parse(string smt)
    {
      std::unique_ptr<ufo::ZFixedPoint <EZ3> > m_fp;
      m_fp.reset (new ZFixedPoint<EZ3> (m_z3));
      ZFixedPoint<EZ3> &fp = *m_fp;
      fp.loadFPfromFile(smt);
      
      string suff(""); //suff("_new");
      
      // a little hack here
      
      for (auto &a : fp.m_rels)
      {
        if (a->arity() == 2)
        {
          failDecl = a->arg(0);
        }
        else if (invVars[a->arg(0)].size() == 0)
        {
          decls.insert(a);
          for (int i = 1; i < a->arity()-1; i++)
          {
            Expr new_name = mkTerm<string> ("__v__" + to_string(i - 1), m_efac);
            Expr var = bind::intConst(new_name);
            invVars[a->arg(0)].push_back(var);
          }
        }
      }
      
      for (auto &r: fp.m_rules)
      {
        chcs.push_back(HornRuleExt());
        HornRuleExt& hr = chcs.back();

        hr.suffix = suff;
        hr.srcRelation = mk<TRUE>(m_efac);
        Expr rule = r;
        ExprVector args;

        if (isOpX<FORALL>(r))
        {
          rule = r->last();

          for (int i = 0; i < r->arity() - 1; i++)
          {
            Expr var = r->arg(i);
            Expr name = bind::name (r->arg(i));
            Expr new_name = mkTerm<string> (lexical_cast<string> (name.get()) + suff, m_efac);
            Expr var_new = bind::fapp(bind::rename(var, new_name));
            args.push_back(var_new);
          }

          ExprVector actual_vars;
          expr::filter (rule, bind::IsVar(), std::inserter (actual_vars, actual_vars.begin ()));

          assert(actual_vars.size() == args.size());

          for (int i = 0; i < actual_vars.size(); i++)
          {
            string a1 = lexical_cast<string>(bind::name(actual_vars[i]));
            int ind = args.size() - 1 - atoi(a1.substr(1).c_str());
            rule = replaceAll(rule, actual_vars[i], args[ind]);
          }
        }

        if (!isOpX<IMPL>(rule)) rule = mk<IMPL>(mk<TRUE>(m_efac), rule);

        Expr body = rule->arg(0);
        Expr head = rule->arg(1);

        hr.head = head->arg(0);
        hr.dstRelation = head->arg(0)->arg(0);

        ExprVector origSrcVars;
        ExprVector lin;
        preprocess(body, origSrcVars, fp.m_rels, hr.srcRelation, lin);

        hr.isFact = isOpX<TRUE>(hr.srcRelation);
        hr.isQuery = (hr.dstRelation == failDecl);
        hr.isInductive = (hr.srcRelation == hr.dstRelation);
        hr.body = conjoin(lin, m_efac);
        outgs[hr.srcRelation].push_back(chcs.size()-1);

        ExprVector origDstVars;

        if (!hr.isQuery)
        {
          for (auto it = head->args_begin()+1, end = head->args_end(); it != end; ++it)
            origDstVars.push_back(*it);
        }

        hr.assignVarsAndRewrite (origSrcVars, invVars[hr.srcRelation],
                                 origDstVars, invVars[hr.dstRelation]);

        for (auto &a: args)
        {
          bool found = false;
          for (auto &b : hr.dstVars)
          {
            if (a == b) found = true;
          }
          if (! found)
          {
            for (auto &b : hr.srcVars)
            {
              if (a == b) found = true;
            }
          }
          if (!found)
          {
            hr.locVars.push_back(a);
          }
        }
      }
    }

    void print()
    {
      int num = 0;
      for (auto &hr: chcs){
        outs () << "\n=========================\n";
        outs () << "RULE #" << num++ << "\n";
        outs() << "\n body [" << * hr.body << "]  ->  " << * hr.head << "\n";
        outs() << "\n" << * hr.srcRelation << "  ->  " << * hr.dstRelation << "\n";
        
        outs() << "\n  SRC VARS: ";
        for(auto &a: hr.srcVars){
          outs() << *a << ", ";
        }
        outs() << "\n";
        outs() << "  DST VARS: ";
        for(auto a: hr.dstVars){
          outs() << *a << ", ";
        }
        outs() << "\n";
        outs() << "  LOCAL VARS: ";
        for(auto a: hr.locVars){
          outs() << *a << ", ";
        }
        
        outs () << "\n";
  
      }
    }
  };
}


#endif