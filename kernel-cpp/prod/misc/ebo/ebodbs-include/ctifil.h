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

#ifndef ctIFILH

#define DAT_EXTENT	".dat"
#define IDX_EXTENT	".idx"

typedef struct iseg {
	COUNT	soffset,	/* segment offset	*/
		slength,	/* segment length	*/
		segmode;	/* segment mode		*/
	} ISEG;

typedef ISEG ctMEM *	pISEG;

#define ISEG_LEN	6
#define ISEG_PLEN	6

typedef struct iidx {
	COUNT	ikeylen,	/* key length		*/
		ikeytyp,	/* key type		*/
		ikeydup,	/* duplicate flag	*/
		inulkey,	/* null ct_key flag	*/
		iempchr,	/* empty character	*/
		inumseg;	/* number of segments	*/
	pISEG   seg;		/* segment information	*/
	pTEXT   ridxnam;	/* r-tree symbolic name	*/
	} IIDX;

typedef IIDX ctMEM *	pIIDX;

#define IIDX_LEN	12

typedef struct {
	COUNT	c[6];
	LONG	seg;
	LONG	ridxnam;
	} IIDXz;

typedef IIDXz ctMEM *	pIIDXz;

#define IIDX_PLEN	sizeof(IIDXz)

typedef struct ifil {
	pTEXT   pfilnam;	/* file name (w/o ext)	*/
	COUNT	dfilno;		/* data file number	*/
	UCOUNT	dreclen;	/* data record length	*/
	UCOUNT	dxtdsiz;	/* data file ext size	*/
	COUNT	dfilmod;	/* data file mode	*/
	COUNT	dnumidx;	/* number of indices	*/
	UCOUNT	ixtdsiz;	/* index file ext size	*/
	COUNT	ifilmod;	/* index file mode	*/
	pIIDX   ix;		/* index information	*/
	pTEXT   rfstfld;	/* r-tree 1st fld name	*/
	pTEXT   rlstfld;	/* r-tree last fld name	*/
	COUNT	tfilno;		/* temporary file number*/
	} IFIL;

typedef IFIL ctMEM *	pIFIL;

#define IFIL_LEN	14
#define IFIL_PLEN	36

#define ctIFILH

#endif /* ctIFILH */

/* end of ctifil.h */
