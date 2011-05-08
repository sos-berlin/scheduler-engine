insert
       into 'sql: -table tabelle c:/tmp/sossql'
       (a,b,c,d)
                    /*into 'tabbed:c:/tmp/sossql'*/

       select tst_artikel,
              tst_benennung[1] || '|' || tst_benennung[2],
              date_as( tst_dat_anleg, 'yymmdd', 'dd-mon-yyyy' ),
              tst_sichbestand

       from teilestamm
       /*'-cobol-type=g:/e/prod/srp/tst.cob record:g:/e/prod/srp/tstdat.dat'*/
       /*from 'fs:(tcp 192.0.0.1/4001)$sot.ebo.tstdat'*/
       /*     described by cobol 'g:/e/prod/srp/tst.cob' */
       /*, 'record:g:/e/prod/srp/text01.dat' described by cobol 'g:/e/prod/srp/text01.cob'*/
/*
       where tst_schl > '000002         000'
             and tst_artikel != '000003'
             and tst_artikel < '010000'
*/
           /*or ( tst_dat_anleg > '940101' and tst_benennung != '' )*/
           /*and tst_artikel < ''*/

             /*and tst_benennung1 = 'TEST-SST'*/

             /*and tst_schl = text_so_schl
             and text_sprache = 'E'*/
             ;


