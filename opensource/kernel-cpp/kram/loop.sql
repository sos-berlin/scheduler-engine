insert
       into 'sql: -table text c:/tmp/sossql'
       (artikel,index,a,b)
                    /*into 'tabbed:c:/tmp/sossql'*/

       select tst_artikel,
              i,
              tst_b_a[ i ],
              tst_b_b[ i ]

       loop i between 1 and 2

       from teilestamm
/*
       where tst_schl > '000002         000'
             and tst_artikel != '000003'
             and tst_artikel < '010000'
*/
       ;

