(declare-var A (Array Int (Array Int Bool)))
(declare-var b (Array Int Bool))
(declare-var b1 (Array Int Bool))
(declare-var bt (Array Int (Array Int Bool)))
(declare-var bt1 (Array Int (Array Int Bool)))
(declare-var g Int)
(declare-var g1 Int)
(declare-var i Int)
(declare-var i1 Int)
(declare-var j Int)
(declare-var j1 Int)
(declare-var M Int)
(declare-var N Int)

(declare-rel inv1 ((Array Int Bool) (Array Int (Array Int Bool)) Int Int Int))
(declare-rel inv2 ((Array Int Bool) (Array Int (Array Int Bool)) Int Int Int Int))
(declare-rel inv3 ((Array Int Bool) (Array Int (Array Int Bool)) (Array Int (Array Int Int)) Bool Int Int Int))
(declare-rel inv4 ((Array Int Bool) (Array Int (Array Int Bool)) (Array Int (Array Int Int)) Bool Int Int Int Int))
(declare-rel inv5 ((Array Int Bool) (Array Int (Array Int Bool)) (Array Int (Array Int Int)) Bool Int Int Int Int))
(declare-rel inv6 ((Array Int Bool) (Array Int (Array Int Bool)) (Array Int (Array Int Int)) Bool Int Int Int))
(declare-rel fail ())

(rule (inv1 b bt 0 M N))

(rule (=> (and (inv1 b bt i M N) 
  (< i M) (= i1 (+ i 1)) (= b1 (store b i 1)))
  (inv2 b bt i 0 M N)))

(rule (=> (and (inv2 b bt i j M N)
  (< j N)
  (= bt1 (store bt i (store (select bt i) j 1)))
  (= j1 (+ j 1)))
  (inv2 b bt i j1 M N)))

(rule (=> (and (inv2 b bt i j M N)
  (not (< j N)) (= i1 (+ i 1)))
  (inv1 b bt i1 M N)))

(rule (=> (and (inv1 b bt i M N)
  (not (< i M)))
  (inv3 b bt A 1 0 M N)))

(rule (=> (and (inv3 b bt A g i M N) 
  (< i (- M 1)) (= i1 (+ i 1)))
  (inv4 b bt A g i 0 M N)))

(rule (=> (and (inv4 b bt A g i j M N)
  (< j (- N 1))
  (= bt1 (store bt i (store (select bt i) (+ j 1) 
     (and (select (select bt i) j)
          (< (select (select A (+ i 1)) (+ j 1)) (select (select A i) j))
          (< (select (select A i) (+ j 1)) (select (select A i) j))
          (< (select (select A (+ i 1)) j) (select (select A i) j))
     )
  )))
  (= j1 (+ j 1)))
  (inv4 b bt A g i j1 M N)))

(rule (=> (and (inv4 b bt A g i j M N)
  (not (< j (- N 1))))
  (inv5 b bt A g i 0 M N)))

(rule (=> (and (inv5 b bt A g i j M N)
  (< j (- N 1))
  (= b1 (store b i (and (select b i) (select (select bt i) (+ j 1)))))
  (= j1 (+ j 1)))
  (inv5 b bt A g i j1 M N)))

(rule (=> (and (inv5 b bt A g i j M N)
  (not (< j (- N 1)))
  (inv6 b bt A g i M N)))

(rule (=> (and (inv6 b bt A g i M N)
  (= g1 (and g (select b i))) (= i1 (+ i 1)))
  (inv3 b bt A g1 i1 M N)))

(rule (=> (and (inv3 b bt A g i M N) 
  (not (< i (- M 1))) true) fail))

(query fail)
