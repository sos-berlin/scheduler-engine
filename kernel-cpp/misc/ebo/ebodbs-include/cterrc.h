/*
 *	This program is the CONFIDENTIAL and PROPRIETARY property 
 *	of FairCom(R) Corporation. Any unauthorized use, reproduction or
 *	transfer of this computer program is strictly prohibited.
 *
 *      Copyright (c) 1984 - 1990 FairCom Corporation.
 *	This is an unpublished work, and is subject to limited distribution and
 *	restricted disclosure only. ALL RIGHTS RESERVED.
 *
 *			RESTRICTED RIGHTS LEGEND
 *	Use, duplication, or disclosure by the Government is subject to
 *	restrictions set forth in subparagraph (c)(1)(ii) of the Rights in
 * 	Technical Data and Computer Software clause at DFARS 252.227-7013.
 *	FairCom Corporation, 4006 West Broadway, Columbia, MO 65203.
 *
 *	c-tree PLUS(tm)	Version 6.0
 *			Release A2
 *			December 3, 1990
 */

/* USER ERROR CODES */


/* Makros ECB_ERROR und NO_ERROR werden in ecberror.h definiert */
#ifndef ECBERROR_INCL
#include "ecberror.h"
#endif

/*
#define NO_ERROR	0
*/


#define	KDUP_ERR	2	/* key value already exists */

#define	KMAT_ERR	3	/* could not delete since pntr's don't match */
#define	KDEL_ERR	4	/* could not find key to delete */
#define	KBLD_ERR	5	/* cannot call delete w/o verification with  */
				/* duplicate keys */
#define BJMP_ERR	6	/* ctree(...) jump table error */

#define SPAC_ERR	10	/* INTREE parameters require too much space */
#define SPRM_ERR	11	/* bad INTREE parameters */
#define FNOP_ERR	12	/* could not open file: not there or locked */
#define	FUNK_ERR	13	/* unknown file type */
#define FCRP_ERR	14	/* file corrupt at open */
#define FCMP_ERR	15	/* file has been compacted */
#define KCRAT_ERR	16	/* could not create index file */
#define DCRAT_ERR	17	/* could not create data file */
#define KOPN_ERR	18	/* tried to create existing index file */
#define DOPN_ERR	19	/* tried to create existing data file */
#define KMIN_ERR	20	/* key length too large for node size */
#define DREC_ERR	21	/* record length too small */
#define FNUM_ERR	22	/* file number out of range */
#define KMEM_ERR	23	/* illegal index member info */
#define FCLS_ERR	24	/* could not close file */
#define KLNK_ERR	25	/* bad link in deleted node list. REBUILD */
#define FACS_ERR	26	/* file number not active */
#define LBOF_ERR	27	/* drn before beginning of data records */
#define ZDRN_ERR	28	/* zero drn in ADDKEY */
#define ZREC_ERR	29	/* zero drn in data file routine */
#define LEOF_ERR	30	/* drn exceeds logical end of file */
#define DELFLG_ERR	31	/* flag not set on record in delete chain */
#define	DDRN_ERR	32	/* attempt to delete record twice in a row */
#define DNUL_ERR	33	/* attempt to use NULL ptr in read/write */
#define PRDS_ERR	34	/* predecessor repeat attempts exhausted */
#define SEEK_ERR	35	/* seek error:  check sysiocod value  */
#define READ_ERR	36	/* read error:  check sysiocod error  */
#define WRITE_ERR	37	/* write error: check sysiocod error */
#define	VRTO_ERR	38	/* could not convert virtual open to actual */
#define FULL_ERR	39	/* no more records availble */
#define KSIZ_ERR	40	/* index node size too large */
#define UDLK_ERR	41	/* could not unlock data record */
#define DLOK_ERR	42	/* could not obtain data record lock */
#define FVER_ERR	43	/* version incompatibility */
#define OSRL_ERR	44	/* data file serial number overflow */
#define KLEN_ERR	45	/* key length exceeds MAXLEN parameter */
#define	FUSE_ERR	46	/* file number already in use.		  */
#define FINT_ERR	47	/* c-tree has not been initialized	  */
#define FMOD_ERR	48	/* operation incompatible with type of file */
#define	FSAV_ERR	49	/* could not save file			  */
#define LNOD_ERR	50	/* could not lock node */
#define UNOD_ERR	51	/* could not unlock node */
#define KTYP_ERR	52	/* variable length keys disabled in  CTOPTN.H */
#define FTYP_ERR	53	/* file mode inconsistent with c-tree config  */
#define REDF_ERR	54	/* attempt to write a read only file	      */
#define DLTF_ERR	55	/* file deletion failed			      */
#define DLTP_ERR	56	/* file must be opened exclusive for delete   */
#define DADV_ERR	57	/* proper lock is not held (CHECKLOCK/READ)   */
#define KLOD_ERR	58	/* LOADKEY called with incorrect key number.  */
				/* You cannot continue.			      */
#define KLOR_ERR	59	/* LOADKEY called with key out of  	      */
				/* order. You may skip this key & continue.   */
#define KFRC_ERR	60	/* fractal out of range			      */
#define CTNL_ERR	61	/* NULL fcb detected during I/O		      */
#define LERR_ERR	62	/* File must be opened exclusively	      */
#define RSER_ERR	63	/* start file / log file serial number error  */
#define RLEN_ERR	64	/* file name too long in log file	      */
#define RMEM_ERR	65	/* not enough memory during tran processing   */
#define RCHK_ERR	66	/* log file entry failed to find checkpoint   */
#define RENF_ERR	67	/* could not rename file		      */
#define LALC_ERR	68	/* could not allocate memory for control list */
#define BNOD_ERR	69	/* node does not belong to index */
#define TEXS_ERR	70	/* transaction already pending */
#define TNON_ERR	71	/* no active transaction */
#define TSHD_ERR	72	/* no space for shadow buffer */
#define TLOG_ERR	73	/* LOGFIL encountered during shadow only */
#define TRAC_ERR	74	/* recovery: two active tran for user */
#define TROW_ERR	75	/* recovery: bad tran owner */
#define TBAD_ERR	76	/* recovery: bad tran type */
#define TRNM_ERR	77	/* recovery: file name too long */
#define TABN_ERR	78	/* transaction abandoned: too many log extents*/
#define FLOG_ERR	79	/* could not log file opn/cre/cls/del */
#define BKEY_ERR	80	/* NULL target or bad keyno */
#define ATRN_ERR	81	/* transaction allocation error */
#define UALC_ERR	82	/* user allocation error */
#define IALC_ERR	83	/* isam allocation error */
#define MUSR_ERR	84	/* maximum users exceeded */
/* .............	85	   not in use ..................... */
#define DEAD_ERR	86	/* dead lock detected */
#define QIET_ERR	87	/* system not quiet: files in use */
#define LMEM_ERR	88	/* linked list memory allocation error */
#define TMEM_ERR	89	/* memory allocation during tran processing */
#define NQUE_ERR	90	/* could not create queue */
#define QWRT_ERR	91	/* queue write error */
#define QMRT_ERR	92	/* queue memory error during write */
#define QRED_ERR	93	/* queue read error */
#define PNDG_ERR	94	/* pending error: cannot save or commit tran */
#define STSK_ERR	95	/* could not start task */
#define LOPN_ERR	96	/* start-file/log open error */
#define SUSR_ERR	97	/* bad user handle */
#define BTMD_ERR	98	/* bad transaction mode */
#define TTYP_ERR	99	/* transaction type / filmod conflict */

#define	ICUR_ERR	100	/* no current record for isam datno  */
#define	INOT_ERR	101	/* could not find isam keyno request */
#define INOD_ERR	102	/* could not open ISAM parameter file */
#define IGIN_ERR	103	/* could not read first 5 parameters in ISAM */
				/* parameter file */
#define IFIL_ERR	104	/* too many files in ISAM parameter file */
#define IUND_ERR	105	/* could noy undo ISAM update. Rebuild Files */
#define IDRI_ERR	106	/* could not read data file record in ISAM */
				/* parameter file */
#define IDRK_ERR	107	/* too many keys for data file in ISAM */
				/* parameter file */
#define IMKY_ERR	108	/* incorrect keyno for index member in */
				/* parameter file */
#define IKRS_ERR	109	/* too many key segments defined in ISAM */
				/* parameter file */
#define ISRC_ERR	110	/* could not read segment record in ISAM */
				/* parameter file */
#define	IKRI_ERR	111	/* could not read index file record in ISAM */
				/* parameter file */
#define IPND_ERR	112	/* LKISAM(ENABLE) found pending locks	   */
#define INOL_ERR	113	/* no memory for user lock table */
#define IRED_ERR	114	/* 1st byte of data record eqauls delete flag */
				/* or bad variable length record mark	      */
#define ISLN_ERR	115	/* key segments do not match key length */
#define IMOD_ERR	116	/* bad mode parameter			   */
#define	IMRI_ERR	117	/* could not read index member record	   */
#define SKEY_ERR	118	/* NXTSET called before FRSSET for keyno   */
#define SKTY_ERR	119	/* FRSSET called for index with wrong keytyp */

#define RRLN_ERR	120	/* data record length exceeds rebuild max  */
#define RSPC_ERR	121	/* not enough space for sort area.	   */
#define RMOD_ERR	122	/* attempt to change fixed vs variable len */
#define	RVHD_ERR	123	/* var length header has bad record mark   */
#define INIX_ERR	124	/* # of indices does not match (OPNIFIL)   */
#define IINT_ERR	125	/* c-tree already initialized		   */

#define ABDR_ERR	126	/* bad directory path get		    */
#define ARQS_ERR	127	/* could not send request		    */
#define ARSP_ERR	128	/* could not receive answer		    */
#define NINT_ERR	129	/* c-tree not initialized		    */
#define AFNM_ERR	130	/* null file name pointer in OPNFIL	    */
#define AFLN_ERR	131	/* file name length exceeds msg size	    */
#define ASPC_ERR	132	/* no room for application message buffer   */
#define ASKY_ERR	133	/* could not identify server: is it active? */
#define ASID_ERR	134	/* could not get servers message id	    */
#define AAID_ERR	135	/* could not allocate application id	    */
#define AMST_ERR	136	/* could not get application msg status	    */
#define AMQZ_ERR	137	/* could not set message appl msg size	    */
#define AMRD_ERR	138	/* could not get rid of application msg	    */ 
#define ABNM_ERR	139	/* badly formed file name		    */
#define VMAX_ERR	140	/* variable record length too long */
#define AMSG_ERR	141	/* required message size exceeds maximum    */

#define SMXL_ERR	142	/* application MAXLEN > server's MAXLEN     */
#define SHND_ERR	143	/* communications handler not installed	    */
#define QMEM_ERR	144	/* application could not id output queue    */
#define ALOG_ERR	145	/* no message space. was login ok?	    */
#define	VDLK_ERR	146	/* could not update free space info */
#define VDLFLG_ERR	147	/* space to be reused is not marked deleted */
#define	VLEN_ERR	148	/* WRTVREC cannot fit record at recbyt */
#define	VRLN_ERR	149	/* varlen less than minimum in ADDVREC */
#define SHUT_ERR	150	/* server is shutting down		    */
#define STRN_ERR	151	/* could not shut down. transactions pending*/
#define LEXT_ERR	152	/* could not extend logfile */
#define	VBSZ_ERR	153	/* buffer too small */
#define	VRCL_ERR	154	/* zero length record in REDVREC */
#define SYST_ERR	155	/* native system failure */
#define	VFLG_ERR	158	/* REDVREC record not marked active */
#define	VPNT_ERR	159	/* zero recbyt value */
#define ITIM_ERR	160	/* multi-user interefernce: index informa-  */
				/* tion updated by the time user got to	*/
				/* actual data record. */
#define SINA_ERR	161	/* user appears inactive		    */
#define SGON_ERR	162	/* server has gone away			    */
#define SFRE_ERR	163	/* no more room in server lock table	    */
#define SFIL_ERR	164	/* file number out of range		    */
#define SNFB_ERR	165	/* no file control block available	    */
#define SNMC_ERR	166	/* no more ct file control blocks in server */
#define SRQS_ERR	167	/* could not read request		    */
#define SRSP_ERR	168	/* could not send answer		    */
#define TCRE_ERR	169	/* create file already opened (in recovery) */

#define SFUN_ERR	170	/* bad function number */
#define SMSG_ERR	171	/* application msg size exceeds server size */
#define SSPC_ERR	172	/* could not allocate server msg buffer	    */
#define SSKY_ERR	173	/* could not identify server		    */
#define SSID_ERR	174	/* could not get server message id	    */
#define SAMS_ERR	175	/* server could not allocate user msg area  */	
#define SMST_ERR	176	/* could not get server msg status	    */
#define SMQZ_ERR	177	/* could not set message server msg size    */
#define SINM_ERR	178	/* unexpected file# assigned to [si] in rcv */ 
#define SOUT_ERR	179	/* server is at full user capacity	    */



#define IKRU_ERR	180	/* could not read symbolic key name	  */
#define IKMU_ERR	181	/* could not get mem for key symb name	  */
#define IKSR_ERR	182	/* no room for sort key. increase MAXFIL  */
#define IDRU_ERR	183	/* could not read file field number values*/
#define ISDP_ERR	184	/* attempt to reallocate set space	  */
#define ISAL_ERR	185	/* not enough memory for multiple sets	  */
#define ISNM_ERR	186	/* set number out of range		  */
#define IRBF_ERR	187	/* null buffer in rtread.c		  */
#define ITBF_ERR	188	/* null target buffer in rtread.c	  */
#define IJSK_ERR	189	/* join_to skip				  */
#define IJER_ERR	190	/* join_to error			  */
#define IJNL_ERR	191	/* join_to null fill			  */
#define IDSK_ERR	192	/* detail_for skip			  */
#define IDER_ERR	193	/* detail_for error			  */
#define IDNL_ERR	194	/* detail_for null fill			  */
#define IDMU_ERR	195	/* could not get mem for dat symb name	  */
#define ITML_ERR	196	/* exceeded RETRY_LIMIT in RTREAD.C	  */

#define IMEM_ERR	197	/* could net get memory for ifil block	  */
#define BIFL_ERR	198	/* improper ifil block			  */
#define NSCH_ERR	199	/* schema not defined for data file	  */

#define RCRE_ERR	400	/* resource already enabled		    */
#define RNON_ERR	401	/* resources not enabled		    */
#define RXCL_ERR	402	/* file must be exclusive to enable res	    */
#define RZRO_ERR	403	/* empty resource id			    */
#define RBUF_ERR	404	/* output buffer to small		    */
#define RDUP_ERR	405	/* resource id already added		    */
#define RCSE_ERR	406	/* bad resource search mode		    */
#define RRED_ERR	407	/* attempt to get non-resource info	    */
#define RNOT_ERR	408	/* resource not found			    */

#define USTP_ERR	410	/* user not active			    */
#define BSUP_ERR	411	/* not a superfile			    */

#define SDIR_ERR	413	/* superfile host not opened		    */
#define SNST_ERR	414	/* cannot nest superfiles		    */
#define SADD_ERR	415	/* illegal ADDKEY to superfile		    */
#define SDEL_ERR	416	/* illegal DELBLD to superfile		    */
#define SPAG_ERR	417	/* cache page size error		    */
#define SNAM_ERR	418	/* max name inconsistency		    */

#define TPND_ERR	420	/* key update with pending transaction	    */
#define BTFL_ERR	421	/* filter not supported yet		    */
#define BTFN_ERR	422	/* other functions not sup		    */
#define BTIC_ERR	423	/* incomplete				    */
#define BTAD_ERR	424	/* add list err				    */
#define BTIP_ERR	425	/* batch in progress			    */
#define BTNO_ERR	426	/* no batch active			    */
#define BTST_ERR	427	/* status info already returned		    */
#define BTMT_ERR	428	/* no more info, batch cancelled	    */
#define BTBZ_ERR	429	/* bufsiz too small for record		    */
#define BTRQ_ERR	430	/* request is empty			    */

#define FLEN_ERR	432	/* fixed length string requires len in DODA */
#define SSCH_ERR	433	/* segment def inconsistent with schema	    */
#define DLEN_ERR	434	/* very long def block not supported	    */
#define FMEM_ERR	435	/* file def memory error		    */
#define DNUM_ERR	436	/* bad def number			    */
#define DADR_ERR	437	/* defptr NULL during GETDEFBLK		    */
#define	DZRO_ERR	438	/* requested def blk is empty		    */
#define DCNV_ERR	439	/* no conversion routine for Def'n Block    */
#define DDDM_ERR	440	/* dynamic dump already in progress	    */
#define DMEM_ERR	441	/* no memory for dynamic dump file buffer   */
#define DAVL_ERR	442	/* one or more files not available for dump */
#define DSIZ_ERR	443	/* file length discrepancy		    */
#define DCRE_ERR	444	/* could not create file during dump rcv    */
#define SDAT_ERR	445	/* not enough data to assemble key value    */
#define BMOD_ERR	446	/* bad key segment mode			    */
#define BOWN_ERR	447	/* only the file's owner can perform op	    */
#define DEFP_ERR	448	/* permission to set file definition denied */

#define LUID_ERR	450	/* invalid user id			    */
#define LPWD_ERR	451	/* invalid password			    */
#define LSRV_ERR	452	/* server could not process user/acct info  */
#define NSRV_ERR	453	/* no such server			    */
#define SGRP_ERR	455	/* user does not belong to group	    */
#define SACS_ERR	456	/* group access denied			    */
#define SPWD_ERR	457	/* file password invalid		    */
#define SWRT_ERR	458	/* write permission not granted		    */
#define SDLT_ERR	459	/* file delete permission denied	    */
#define SRES_ERR	460	/* resource not enabled			    */
#define SPER_ERR	461	/* bad permission flag			    */
#define SHDR_ERR	462	/* no directory found in superfile	    */
#define UQID_ERR	463	/* file id uniqueness error		    */

#define SORT_ERR	370	/* sort base: errors SORT_ERR + 101 thru 126*/
				/* see CTSORT.C for error listing	    */
/* .... reserves	471 to 496 ........................................ */

#define SQLINIT_ERR		500
#define SQLCONNECT_ERR		501
#define SQL_REQUEST_ERROR	502
#define SQL_INVALID_CONTINUE	503
#define NSQL_ERR		504

#define ROLLBACK_ERR		799
#define HERZ_ERR                800
#define PID_ERR                 801
/* end of cterrc.h */
