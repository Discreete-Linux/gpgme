/* guiddef.h 

 Copyright (c) 1993-2000 the Wine project authors (see the file WINE-AUTHORS
 for a complete list)
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/* already defined bu Mingw32/cpd
#ifndef GUID_DEFINED
#define GUID_DEFINED
typedef struct _GUID
{
    unsigned long  Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char  Data4[ 8 ];
} GUID;
#endif
*/

#undef DEFINE_GUID

#ifdef INITGUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        const GUID name = \
	{ l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
#else
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    extern const GUID name
#endif

#define DEFINE_OLEGUID(name, l, w1, w2) \
	DEFINE_GUID(name, l, w1, w2, 0xC0,0,0,0,0,0,0,0x46)

#ifndef _GUIDDEF_H_
#define _GUIDDEF_H_

/*   typedef GUID *LPGUID;
     typedef GUID CLSID,*LPCLSID; */
typedef GUID IID,*LPIID;
typedef GUID FMTID,*LPFMTID;

#if defined(__cplusplus) && !defined(CINTERFACE)
#define REFGUID             const GUID &
#define REFCLSID            const CLSID &
#define REFIID              const IID &
#define REFFMTID            const FMTID &
#else /* !defined(__cplusplus) && !defined(CINTERFACE) */
#define REFGUID             const GUID* const
#define REFCLSID            const CLSID* const
#define REFIID              const IID* const
#define REFFMTID            const FMTID* const
#endif /* !defined(__cplusplus) && !defined(CINTERFACE) */

#if defined(__cplusplus) && !defined(CINTERFACE)
#define IsEqualGUID(rguid1, rguid2) (!memcmp(&(rguid1), &(rguid2), sizeof(GUID)))
#else /* defined(__cplusplus) && !defined(CINTERFACE) */
#define IsEqualGUID(rguid1, rguid2) (!memcmp(rguid1, rguid2, sizeof(GUID)))
#endif /* defined(__cplusplus) && !defined(CINTERFACE) */
#define IsEqualIID(riid1, riid2) IsEqualGUID(riid1, riid2)
#define IsEqualCLSID(rclsid1, rclsid2) IsEqualGUID(rclsid1, rclsid2)

#if defined(__cplusplus) && !defined(CINTERFACE)
inline bool operator==(const GUID& guidOne, const GUID& guidOther)
{
    return !memcmp(&guidOne,&guidOther,sizeof(GUID));
}
inline bool operator!=(const GUID& guidOne, const GUID& guidOther)
{
    return !(guidOne == guidOther);
}
#endif 

extern const IID GUID_NULL;
#define IID_NULL            GUID_NULL
#define CLSID_NULL GUID_NULL
#define FMTID_NULL          GUID_NULL
   
#endif /* _GUIDDEF_H_ */
