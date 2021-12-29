# Dim's Pascal Pythagoras theorem benchmark
Let `a = pyth(b, c)` be the syntax of Pythagoras theorem such that if `a = pyth(3, 4)` then `a` equals to 5.  
Let `list[n]` be the syntax of retrieving the n-th element in a list such that `[1, 2][0] -> 1`.  
Let the first index of any list be 0. So `list[0]` accesses the first element in the list.


The benchmark works like this:

1. Declare n = 2
2. Repeat until time measured >5s and step is 1:
   1. Declare list `[1..n]`
   2. Start measuring time
   3. Repeat until size of `a` is 1:
      1. For `i` = 0 to `(size of list)-1`:
         * `a[i] = pyth(a[i], a[i+1])`
      2. Remove last element in `a`
   4. Stop measuring time
   5. n += 1

`n` is the final score.

More details:
* `Target` is the time used to determine whether a test is successful or not.
  * This is defined as 5000ms.
  * If a test took >5000ms to execute, it is considered a fail and will step down.
* `Step limit` is the time used to determine whether the next test should step up or not.
  * If `Step limit` < current test's elapsed time < `Target` then step up.