#include "kangxi.h"

/*
   Table of the Kang-Xi Radicals to the ideographs they represent
*/

/* form:  
  radical -> normal
*/

UChar  gKangXiRadicalTable[] = 
{0x2F00	,
	0x4E00
,0x2F01	,
	0x4E28
,0x2F02	,
	0x4E36
,0x2F03	,
	0x4E3F
,0x2F04	,
	0x4E59
,0x2F05	,
	0x4E85
,0x2F06	,
	0x4E8C
,0x2F07	,
	0x4EA0
,0x2F08	,
	0x4EBA
,0x2F09	,
	0x513F
,0x2F0A	,
	0x5165
,0x2F0B	,
	0x516B
,0x2F0C	,
	0x5182
,0x2F0D	,
	0x5196
,0x2F0E	,
	0x51AB
,0x2F0F	,
	0x51E0
,0x2F10	,
	0x51F5
,0x2F11	,
	0x5200
,0x2F12	,
	0x529B
,0x2F13	,
	0x52F9
,0x2F14	,
	0x5315
,0x2F15	,
	0x531A
,0x2F16	,
	0x5338
,0x2F17	,
	0x5341
,0x2F18	,
	0x535C
,0x2F19	,
	0x5369
,0x2F1A	,
	0x5382
,0x2F1B	,
	0x53B6
,0x2F1C	,
	0x53C8
,0x2F1D	,
	0x53E3
,0x2F1E	,
	0x56D7
,0x2F1F	,
	0x571F
,0x2F20	,
	0x58EB
,0x2F21	,
	0x5902
,0x2F22	,
	0x590A
,0x2F23	,
	0x5915
,0x2F24	,
	0x5927
,0x2F25	,
	0x5973
,0x2F26	,
	0x5B50
,0x2F27	,
	0x5B80
,0x2F28	,
	0x5BF8
,0x2F29	,
	0x5C0F
,0x2F2A	,
	0x5C22
,0x2F2B	,
	0x5C38
,0x2F2C	,
	0x5C6E
,0x2F2D	,
	0x5C71
,0x2F2E	,
	0x5DDB
,0x2F2F	,
	0x5DE5
,0x2F30	,
	0x5DF1
,0x2F31	,
	0x5DFE
,0x2F32	,
	0x5E72
,0x2F33	,
	0x5E7A
,0x2F34	,
	0x5E7F
,0x2F35	,
	0x5EF4
,0x2F36	,
	0x5EFE
,0x2F37	,
	0x5F0B
,0x2F38	,
	0x5F13
,0x2F39	,
	0x5F50
,0x2F3A	,
	0x5F61
,0x2F3B	,
	0x5F73
,0x2F3C	,
	0x5FC3
,0x2F3D	,
	0x6208
,0x2F3E	,
	0x6236
,0x2F3F	,
	0x624B
,0x2F40	,
	0x652F
,0x2F41	,
	0x6534
,0x2F42	,
	0x6587
,0x2F43	,
	0x6597
,0x2F44	,
	0x65A4
,0x2F45	,
	0x65B9
,0x2F46	,
	0x65E0
,0x2F47	,
	0x65E5
,0x2F48	,
	0x66F0
,0x2F49	,
	0x6708
,0x2F4A	,
	0x6728
,0x2F4B	,
	0x6B20
,0x2F4C	,
	0x6B62
,0x2F4D	,
	0x6B79
,0x2F4E	,
	0x6BB3
,0x2F4F	,
	0x6BCB
,0x2F50	,
	0x6BD4
,0x2F51	,
	0x6BDB
,0x2F52	,
	0x6C0F
,0x2F53	,
	0x6C14
,0x2F54	,
	0x6C34
,0x2F55	,
	0x706B
,0x2F56	,
	0x722A
,0x2F57	,
	0x7236
,0x2F58	,
	0x723B
,0x2F59	,
	0x723F
,0x2F5A	,
	0x7247
,0x2F5B	,
	0x7259
,0x2F5C	,
	0x725B
,0x2F5D	,
	0x72AC
,0x2F5E	,
	0x7384
,0x2F5F	,
	0x7389
,0x2F60	,
	0x74DC
,0x2F61	,
	0x74E6
,0x2F62	,
	0x7518
,0x2F63	,
	0x751F
,0x2F64	,
	0x7528
,0x2F65	,
	0x7530
,0x2F66	,
	0x758B
,0x2F67	,
	0x7592
,0x2F68	,
	0x7676
,0x2F69	,
	0x767D
,0x2F6A	,
	0x76AE
,0x2F6B	,
	0x76BF
,0x2F6C	,
	0x76EE
,0x2F6D	,
	0x77DB
,0x2F6E	,
	0x77E2
,0x2F6F	,
	0x77F3
,0x2F70	,
	0x793A
,0x2F71	,
	0x79B8
,0x2F72	,
	0x79BE
,0x2F73	,
	0x7A74
,0x2F74	,
	0x7ACB
,0x2F75	,
	0x7AF9
,0x2F76	,
	0x7C73
,0x2F77	,
	0x7CF8
,0x2F78	,
	0x7F36
,0x2F79	,
	0x7F51
,0x2F7A	,
	0x7F8A
,0x2F7B	,
	0x7FBD
,0x2F7C	,
	0x8001
,0x2F7D	,
	0x800C
,0x2F7E	,
	0x8012
,0x2F7F	,
	0x8033
,0x2F80	,
	0x807F
,0x2F81	,
	0x8089
,0x2F82	,
	0x81E3
,0x2F83	,
	0x81EA
,0x2F84	,
	0x81F3
,0x2F85	,
	0x81FC
,0x2F86	,
	0x820C
,0x2F87	,
	0x821B
,0x2F88	,
	0x821F
,0x2F89	,
	0x826E
,0x2F8A	,
	0x8272
,0x2F8B	,
	0x8278
,0x2F8C	,
	0x864D
,0x2F8D	,
	0x866B
,0x2F8E	,
	0x8840
,0x2F8F	,
	0x884C
,0x2F90	,
	0x8863
,0x2F91	,
	0x897E
,0x2F92	,
	0x898B
,0x2F93	,
	0x89D2
,0x2F94	,
	0x8A00
,0x2F95	,
	0x8C37
,0x2F96	,
	0x8C46
,0x2F97	,
	0x8C55
,0x2F98	,
	0x8C78
,0x2F99	,
	0x8C9D
,0x2F9A	,
	0x8D64
,0x2F9B	,
	0x8D70
,0x2F9C	,
	0x8DB3
,0x2F9D	,
	0x8EAB
,0x2F9E	,
	0x8ECA
,0x2F9F	,
	0x8F9B
,0x2FA0	,
	0x8FB0
,0x2FA1	,
	0x8FB5
,0x2FA2	,
	0x9091
,0x2FA3	,
	0x9149
,0x2FA4	,
	0x91C6
,0x2FA5	,
	0x91CC
,0x2FA6	,
	0x91D1
,0x2FA7	,
	0x9577
,0x2FA8	,
	0x9580
,0x2FA9	,
	0x961C
,0x2FAA	,
	0x96B6
,0x2FAB	,
	0x96B9
,0x2FAC	,
	0x96E8
,0x2FAD	,
	0x9751
,0x2FAE	,
	0x975E
,0x2FAF	,
	0x9762
,0x2FB0	,
	0x9769
,0x2FB1	,
	0x97CB
,0x2FB2	,
	0x97ED
,0x2FB3	,
	0x97F3
,0x2FB4	,
	0x9801
,0x2FB5	,
	0x98A8
,0x2FB6	,
	0x98DB
,0x2FB7	,
	0x98DF
,0x2FB8	,
	0x9996
,0x2FB9	,
	0x9999
,0x2FBA	,
	0x99AC
,0x2FBB	,
	0x9AA8
,0x2FBC	,
	0x9AD8
,0x2FBD	,
	0x9ADF
,0x2FBE	,
	0x9B25
,0x2FBF	,
	0x9B2F
,0x2FC0	,
	0x9B32
,0x2FC1	,
	0x9B3C
,0x2FC2	,
	0x9B5A
,0x2FC3	,
	0x9CE5
,0x2FC4	,
	0x9E75
,0x2FC5	,
	0x9E7F
,0x2FC6	,
	0x9EA5
,0x2FC7	,
	0x9EBB
,0x2FC8	,
	0x9EC3
,0x2FC9	,
	0x9ECD
,0x2FCA	,
	0x9ED1
,0x2FCB	,
	0x9EF9
,0x2FCC	,
	0x9EFD
,0x2FCD	,
	0x9F0E
,0x2FCE	,
	0x9F13
,0x2FCF	,
	0x9F20
,0x2FD0	,
	0x9F3B
,0x2FD1	,
	0x9F4A
,0x2FD2	,
	0x9F52
,0x2FD3	,
	0x9F8D
,0x2FD4	,
	0x9F9C
,0x2FD5	,
	0x9FA0,
 0,0 /* end */
};
