# dasmの文法(x86)

```lua
------------------------------------------------------------------------------
-- x86 Template String Description
-- ===============================
--
-- Each template string is a list of [match:]pattern pairs,
-- separated by "|". The first match wins. No match means a
-- bad or unsupported combination of operand modes or sizes.
--
-- The match part and the ":" is omitted if the operation has
-- no operands. Otherwise the first N characters are matched
-- against the mode strings of each of the N operands.
--
-- The mode string for each operand type is (see parseoperand()):
--   Integer register: "rm", +"R" for eax, ax, al, +"C" for cl
--   FP register:      "f",  +"F" for st0
--   Index operand:    "xm", +"O" for [disp] (pure offset)
--   Immediate:        "i",  +"S" for signed 8 bit, +"1" for 1, +"I" for arg, +"P" for pointer
--   Any:              +"J" for valid jump targets
--
-- So a match character "m" (mixed) matches both an integer register
-- and an index operand (to be encoded with the ModRM/SIB scheme).
-- But "r" matches only a register and "x" only an index operand
-- (e.g. for FP memory access operations).
--
-- The operand size match string starts right after the mode match
-- characters and ends before the ":". "dwb" or "qdwb" is assumed, if empty.
-- The effective data size of the operation is matched against this list.
--
-- If only the regular "b", "w", "d", "q", "t" operand sizes are
-- present, then all operands must be the same size. Unspecified sizes
-- are ignored, but at least one operand must have a size or the pattern
-- won't match (use the "byte", "word", "dword", "qword", "tword"
-- operand size overrides. E.g.: mov dword [eax], 1).
--
-- If the list has a "1" or "2" prefix, the operand size is taken
-- from the respective operand and any other operand sizes are ignored.
-- If the list contains only ".", all operand sizes are ignored.
-- If the list has a "/" prefix, the concatenated (mixed) operand sizes
-- are compared to the match.
--
-- E.g. "rrdw" matches for either two dword registers or two word
-- registers. "Fx2dq" matches an st0 operand plus an index operand
-- pointing to a dword (float) or qword (double).
--
-- Every character after the ":" is part of the pattern string:
--   Hex chars are accumulated to form the opcode (left to right).
--   "n"       disables the standard opcode mods
--             (otherwise: -1 for "b", o16 prefix for "w", rex.w for "q")
--   "X"       Force REX.W.
--   "r"/"R"   adds the reg. number from the 1st/2nd operand to the opcode.
--   "m"/"M"   generates ModRM/SIB from the 1st/2nd operand.
--             The spare 3 bits are either filled with the last hex digit or
--             the result from a previous "r"/"R". The opcode is restored.
--
-- All of the following characters force a flush of the opcode:
--   "o"/"O"   stores a pure 32 bit disp (offset) from the 1st/2nd operand.
--   "S"       stores a signed 8 bit immediate from the last operand.
--   "U"       stores an unsigned 8 bit immediate from the last operand.
--   "W"       stores an unsigned 16 bit immediate from the last operand.
--   "i"       stores an operand sized immediate from the last operand.
--   "I"       dito, but generates an action code to optionally modify the opcode (+2) for a signed 8 bit immediate.
--   "J"       generates one of the REL action codes from the last operand.
--
------------------------------------------------------------------------------

-- Template strings for x86 instructions. Ordered by first opcode byte.
-- Unimplemented opcodes (deliberate omissions) are marked with *.
local map_op = {
  -- 00-05: add...
  -- 06: *push es
  -- 07: *pop es
  -- 08-0D: or...
  -- 0E: *push cs
  -- 0F: two byte opcode prefix
  -- 10-15: adc...
  -- 16: *push ss
  -- 17: *pop ss
  -- 18-1D: sbb...
  -- 1E: *push ds
  -- 1F: *pop ds
  -- 20-25: and...
  es_0 =	"26",
  -- 27: *daa
  -- 28-2D: sub...
  cs_0 =	"2E",
  -- 2F: *das
  -- 30-35: xor...
  ss_0 =	"36",
  -- 37: *aaa
  -- 38-3D: cmp...
  ds_0 =	"3E",
  -- 3F: *aas
  inc_1 =	x64 and "m:FF0m" or "rdw:40r|m:FF0m",
  dec_1 =	x64 and "m:FF1m" or "rdw:48r|m:FF1m",
  push_1 =	(x64 and "rq:n50r|rw:50r|mq:nFF6m|mw:FF6m" or "rdw:50r|mdw:FF6m").."|S.:6AS|ib:n6Ai|i.:68i",
  pop_1 =	x64 and "rq:n58r|rw:58r|mq:n8F0m|mw:8F0m" or "rdw:58r|mdw:8F0m",
  -- 60: *pusha, *pushad, *pushaw
  -- 61: *popa, *popad, *popaw
  -- 62: *bound rdw,x
  -- 63: x86: *arpl mw,rw
  movsxd_2 =	x64 and "rm/qd:63rM",
  fs_0 =	"64",
  gs_0 =	"65",
  o16_0 =	"66",
  a16_0 =	not x64 and "67" or nil,
  a32_0 =	x64 and "67",
  -- 68: push idw
  -- 69: imul rdw,mdw,idw
  -- 6A: push ib
  -- 6B: imul rdw,mdw,S
  -- 6C: *insb
  -- 6D: *insd, *insw
  -- 6E: *outsb
  -- 6F: *outsd, *outsw
  -- 70-7F: jcc lb
  -- 80: add... mb,i
  -- 81: add... mdw,i
  -- 82: *undefined
  -- 83: add... mdw,S
  test_2 =	"mr:85Rm|rm:85rM|Ri:A9ri|mi:F70mi",
  -- 86: xchg rb,mb
  -- 87: xchg rdw,mdw
  -- 88: mov mb,r
  -- 89: mov mdw,r
  -- 8A: mov r,mb
  -- 8B: mov r,mdw
  -- 8C: *mov mdw,seg
  lea_2 =	"rx1dq:8DrM",
  -- 8E: *mov seg,mdw
  -- 8F: pop mdw
  nop_0 =	"90",
  xchg_2 =	"Rrqdw:90R|rRqdw:90r|rm:87rM|mr:87Rm",
  cbw_0 =	"6698",
  cwde_0 =	"98",
  cdqe_0 =	"4898",
  cwd_0 =	"6699",
  cdq_0 =	"99",
  cqo_0 =	"4899",
  -- 9A: *call iw:idw
  wait_0 =	"9B",
  fwait_0 =	"9B",
  pushf_0 =	"9C",
  pushfd_0 =	not x64 and "9C",
  pushfq_0 =	x64 and "9C",
  popf_0 =	"9D",
  popfd_0 =	not x64 and "9D",
  popfq_0 =	x64 and "9D",
  sahf_0 =	"9E",
  lahf_0 =	"9F",
  mov_2 =	"OR:A3o|RO:A1O|mr:89Rm|rm:8BrM|rib:nB0ri|ridw:B8ri|mi:C70mi",
  movsb_0 =	"A4",
  movsw_0 =	"66A5",
  movsd_0 =	"A5",
  cmpsb_0 =	"A6",
  cmpsw_0 =	"66A7",
  cmpsd_0 =	"A7",
  -- A8: test Rb,i
  -- A9: test Rdw,i
  stosb_0 =	"AA",
  stosw_0 =	"66AB",
  stosd_0 =	"AB",
  lodsb_0 =	"AC",
  lodsw_0 =	"66AD",
  lodsd_0 =	"AD",
  scasb_0 =	"AE",
  scasw_0 =	"66AF",
  scasd_0 =	"AF",
  -- B0-B7: mov rb,i
  -- B8-BF: mov rdw,i
  -- C0: rol... mb,i
  -- C1: rol... mdw,i
  ret_1 =	"i.:nC2W",
  ret_0 =	"C3",
  -- C4: *les rdw,mq
  -- C5: *lds rdw,mq
  -- C6: mov mb,i
  -- C7: mov mdw,i
  -- C8: *enter iw,ib
  leave_0 =	"C9",
  -- CA: *retf iw
  -- CB: *retf
  int3_0 =	"CC",
  int_1 =	"i.:nCDU",
  into_0 =	"CE",
  -- CF: *iret
  -- D0: rol... mb,1
  -- D1: rol... mdw,1
  -- D2: rol... mb,cl
  -- D3: rol... mb,cl
  -- D4: *aam ib
  -- D5: *aad ib
  -- D6: *salc
  -- D7: *xlat
  -- D8-DF: floating point ops
  -- E0: *loopne
  -- E1: *loope
  -- E2: *loop
  -- E3: *jcxz, *jecxz
  -- E4: *in Rb,ib
  -- E5: *in Rdw,ib
  -- E6: *out ib,Rb
  -- E7: *out ib,Rdw
  call_1 =	x64 and "mq:nFF2m|J.:E8nJ" or "md:FF2m|J.:E8J",
  jmp_1 =	x64 and "mq:nFF4m|J.:E9nJ" or "md:FF4m|J.:E9J", -- short: EB
  -- EA: *jmp iw:idw
  -- EB: jmp ib
  -- EC: *in Rb,dx
  -- ED: *in Rdw,dx
  -- EE: *out dx,Rb
  -- EF: *out dx,Rdw
  lock_0 =	"F0",
  int1_0 =	"F1",
  repne_0 =	"F2",
  repnz_0 =	"F2",
  rep_0 =	"F3",
  repe_0 =	"F3",
  repz_0 =	"F3",
  -- F4: *hlt
  cmc_0 =	"F5",
  -- F6: test... mb,i; div... mb
  -- F7: test... mdw,i; div... mdw
  clc_0 =	"F8",
  stc_0 =	"F9",
  -- FA: *cli
  cld_0 =	"FC",
  std_0 =	"FD",
  -- FE: inc... mb
  -- FF: inc... mdw

  -- misc ops
  not_1 =	"m:F72m",
  neg_1 =	"m:F73m",
  mul_1 =	"m:F74m",
  imul_1 =	"m:F75m",
  div_1 =	"m:F76m",
  idiv_1 =	"m:F77m",

  imul_2 =	"rmqdw:0FAFrM|rIqdw:69rmI|rSqdw:6BrmS|riqdw:69rmi",
  imul_3 =	"rmIqdw:69rMI|rmSqdw:6BrMS|rmiqdw:69rMi",

  movzx_2 =	"rm/db:0FB6rM|rm/qb:|rm/wb:0FB6rM|rm/dw:0FB7rM|rm/qw:",
  movsx_2 =	"rm/db:0FBErM|rm/qb:|rm/wb:0FBErM|rm/dw:0FBFrM|rm/qw:",

  bswap_1 =	"rqd:0FC8r",
  bsf_2 =	"rmqdw:0FBCrM",
  bsr_2 =	"rmqdw:0FBDrM",
  bt_2 =	"mrqdw:0FA3Rm|miqdw:0FBA4mU",
  btc_2 =	"mrqdw:0FBBRm|miqdw:0FBA7mU",
  btr_2 =	"mrqdw:0FB3Rm|miqdw:0FBA6mU",
  bts_2 =	"mrqdw:0FABRm|miqdw:0FBA5mU",

  shld_3 =	"mriqdw:0FA4RmU|mrC/qq:0FA5Rm|mrC/dd:|mrC/ww:",
  shrd_3 =	"mriqdw:0FACRmU|mrC/qq:0FADRm|mrC/dd:|mrC/ww:",

  rdtsc_0 =	"0F31", -- P1+
  rdpmc_0 =	"0F33", -- P6+
  cpuid_0 =	"0FA2", -- P1+

  -- floating point ops
  fst_1 =	"ff:DDD0r|xd:D92m|xq:nDD2m",
  fstp_1 =	"ff:DDD8r|xd:D93m|xq:nDD3m|xt:DB7m",
  fld_1 =	"ff:D9C0r|xd:D90m|xq:nDD0m|xt:DB5m",

  fpop_0 =	"DDD8", -- Alias for fstp st0.

  fist_1 =	"xw:nDF2m|xd:DB2m",
  fistp_1 =	"xw:nDF3m|xd:DB3m|xq:nDF7m",
  fild_1 =	"xw:nDF0m|xd:DB0m|xq:nDF5m",

  fxch_0 =	"D9C9",
  fxch_1 =	"ff:D9C8r",
  fxch_2 =	"fFf:D9C8r|Fff:D9C8R",

  fucom_1 =	"ff:DDE0r",
  fucom_2 =	"Fff:DDE0R",
  fucomp_1 =	"ff:DDE8r",
  fucomp_2 =	"Fff:DDE8R",
  fucomi_1 =	"ff:DBE8r", -- P6+
  fucomi_2 =	"Fff:DBE8R", -- P6+
  fucomip_1 =	"ff:DFE8r", -- P6+
  fucomip_2 =	"Fff:DFE8R", -- P6+
  fcomi_1 =	"ff:DBF0r", -- P6+
  fcomi_2 =	"Fff:DBF0R", -- P6+
  fcomip_1 =	"ff:DFF0r", -- P6+
  fcomip_2 =	"Fff:DFF0R", -- P6+
  fucompp_0 =	"DAE9",
  fcompp_0 =	"DED9",

  fldenv_1 =	"x.:D94m",
  fnstenv_1 =	"x.:D96m",
  fstenv_1 =	"x.:9BD96m",
  fldcw_1 =	"xw:nD95m",
  fstcw_1 =	"xw:n9BD97m",
  fnstcw_1 =	"xw:nD97m",
  fstsw_1 =	"Rw:n9BDFE0|xw:n9BDD7m",
  fnstsw_1 =	"Rw:nDFE0|xw:nDD7m",
  fclex_0 =	"9BDBE2",
  fnclex_0 =	"DBE2",

  fnop_0 =	"D9D0",
  -- D9D1-D9DF: unassigned

  fchs_0 =	"D9E0",
  fabs_0 =	"D9E1",
  -- D9E2: unassigned
  -- D9E3: unassigned
  ftst_0 =	"D9E4",
  fxam_0 =	"D9E5",
  -- D9E6: unassigned
  -- D9E7: unassigned
  fld1_0 =	"D9E8",
  fldl2t_0 =	"D9E9",
  fldl2e_0 =	"D9EA",
  fldpi_0 =	"D9EB",
  fldlg2_0 =	"D9EC",
  fldln2_0 =	"D9ED",
  fldz_0 =	"D9EE",
  -- D9EF: unassigned

  f2xm1_0 =	"D9F0",
  fyl2x_0 =	"D9F1",
  fptan_0 =	"D9F2",
  fpatan_0 =	"D9F3",
  fxtract_0 =	"D9F4",
  fprem1_0 =	"D9F5",
  fdecstp_0 =	"D9F6",
  fincstp_0 =	"D9F7",
  fprem_0 =	"D9F8",
  fyl2xp1_0 =	"D9F9",
  fsqrt_0 =	"D9FA",
  fsincos_0 =	"D9FB",
  frndint_0 =	"D9FC",
  fscale_0 =	"D9FD",
  fsin_0 =	"D9FE",
  fcos_0 =	"D9FF",

  -- SSE, SSE2
  andnpd_2 =	"rmo:660F55rM",
  andnps_2 =	"rmo:0F55rM",
  andpd_2 =	"rmo:660F54rM",
  andps_2 =	"rmo:0F54rM",
  clflush_1 =	"x.:0FAE7m",
  cmppd_3 =	"rmio:660FC2rMU",
  cmpps_3 =	"rmio:0FC2rMU",
  cmpsd_3 =	"rrio:F20FC2rMU|rxi/oq:",
  cmpss_3 =	"rrio:F30FC2rMU|rxi/od:",
  comisd_2 =	"rro:660F2FrM|rx/oq:",
  comiss_2 =	"rro:0F2FrM|rx/od:",
  cvtdq2pd_2 =	"rro:F30FE6rM|rx/oq:",
  cvtdq2ps_2 =	"rmo:0F5BrM",
  cvtpd2dq_2 =	"rmo:F20FE6rM",
  cvtpd2ps_2 =	"rmo:660F5ArM",
  cvtpi2pd_2 =	"rx/oq:660F2ArM",
  cvtpi2ps_2 =	"rx/oq:0F2ArM",
  cvtps2dq_2 =	"rmo:660F5BrM",
  cvtps2pd_2 =	"rro:0F5ArM|rx/oq:",
  cvtsd2si_2 =	"rr/do:F20F2DrM|rr/qo:|rx/dq:|rxq:",
  cvtsd2ss_2 =	"rro:F20F5ArM|rx/oq:",
  cvtsi2sd_2 =	"rm/od:F20F2ArM|rm/oq:F20F2ArXM",
  cvtsi2ss_2 =	"rm/od:F30F2ArM|rm/oq:F30F2ArXM",
  cvtss2sd_2 =	"rro:F30F5ArM|rx/od:",
  cvtss2si_2 =	"rr/do:F30F2DrM|rr/qo:|rxd:|rx/qd:",
  cvttpd2dq_2 =	"rmo:660FE6rM",
  cvttps2dq_2 =	"rmo:F30F5BrM",
  cvttsd2si_2 =	"rr/do:F20F2CrM|rr/qo:|rx/dq:|rxq:",
  cvttss2si_2 =	"rr/do:F30F2CrM|rr/qo:|rxd:|rx/qd:",
  fxsave_1 =	"x.:0FAE0m",
  fxrstor_1 =	"x.:0FAE1m",
  ldmxcsr_1 =	"xd:0FAE2m",
  lfence_0 =	"0FAEE8",
  maskmovdqu_2 = "rro:660FF7rM",
  mfence_0 =	"0FAEF0",
  movapd_2 =	"rmo:660F28rM|mro:660F29Rm",
  movaps_2 =	"rmo:0F28rM|mro:0F29Rm",
  movd_2 =	"rm/od:660F6ErM|rm/oq:660F6ErXM|mr/do:660F7ERm|mr/qo:",
  movdqa_2 =	"rmo:660F6FrM|mro:660F7FRm",
  movdqu_2 =	"rmo:F30F6FrM|mro:F30F7FRm",
  movhlps_2 =	"rro:0F12rM",
  movhpd_2 =	"rx/oq:660F16rM|xr/qo:n660F17Rm",
  movhps_2 =	"rx/oq:0F16rM|xr/qo:n0F17Rm",
  movlhps_2 =	"rro:0F16rM",
  movlpd_2 =	"rx/oq:660F12rM|xr/qo:n660F13Rm",
  movlps_2 =	"rx/oq:0F12rM|xr/qo:n0F13Rm",
  movmskpd_2 =	"rr/do:660F50rM",
  movmskps_2 =	"rr/do:0F50rM",
  movntdq_2 =	"xro:660FE7Rm",
  movnti_2 =	"xrqd:0FC3Rm",
  movntpd_2 =	"xro:660F2BRm",
  movntps_2 =	"xro:0F2BRm",
  movq_2 =	"rro:F30F7ErM|rx/oq:|xr/qo:n660FD6Rm",
  movsd_2 =	"rro:F20F10rM|rx/oq:|xr/qo:nF20F11Rm",
  movss_2 =	"rro:F30F10rM|rx/od:|xr/do:F30F11Rm",
  movupd_2 =	"rmo:660F10rM|mro:660F11Rm",
  movups_2 =	"rmo:0F10rM|mro:0F11Rm",
  orpd_2 =	"rmo:660F56rM",
  orps_2 =	"rmo:0F56rM",
  packssdw_2 =	"rmo:660F6BrM",
  packsswb_2 =	"rmo:660F63rM",
  packuswb_2 =	"rmo:660F67rM",
  paddb_2 =	"rmo:660FFCrM",
  paddd_2 =	"rmo:660FFErM",
  paddq_2 =	"rmo:660FD4rM",
  paddsb_2 =	"rmo:660FECrM",
  paddsw_2 =	"rmo:660FEDrM",
  paddusb_2 =	"rmo:660FDCrM",
  paddusw_2 =	"rmo:660FDDrM",
  paddw_2 =	"rmo:660FFDrM",
  pand_2 =	"rmo:660FDBrM",
  pandn_2 =	"rmo:660FDFrM",
  pause_0 =	"F390",
  pavgb_2 =	"rmo:660FE0rM",
  pavgw_2 =	"rmo:660FE3rM",
  pcmpeqb_2 =	"rmo:660F74rM",
  pcmpeqd_2 =	"rmo:660F76rM",
  pcmpeqw_2 =	"rmo:660F75rM",
  pcmpgtb_2 =	"rmo:660F64rM",
  pcmpgtd_2 =	"rmo:660F66rM",
  pcmpgtw_2 =	"rmo:660F65rM",
  pextrw_3 =	"rri/do:660FC5rMU|xri/wo:660F3A15nRmU", -- Mem op: SSE4.1 only.
  pinsrw_3 =	"rri/od:660FC4rMU|rxi/ow:",
  pmaddwd_2 =	"rmo:660FF5rM",
  pmaxsw_2 =	"rmo:660FEErM",
  pmaxub_2 =	"rmo:660FDErM",
  pminsw_2 =	"rmo:660FEArM",
  pminub_2 =	"rmo:660FDArM",
  pmovmskb_2 =	"rr/do:660FD7rM",
  pmulhuw_2 =	"rmo:660FE4rM",
  pmulhw_2 =	"rmo:660FE5rM",
  pmullw_2 =	"rmo:660FD5rM",
  pmuludq_2 =	"rmo:660FF4rM",
  por_2 =	"rmo:660FEBrM",
  prefetchnta_1 = "xb:n0F180m",
  prefetcht0_1 = "xb:n0F181m",
  prefetcht1_1 = "xb:n0F182m",
  prefetcht2_1 = "xb:n0F183m",
  psadbw_2 =	"rmo:660FF6rM",
  pshufd_3 =	"rmio:660F70rMU",
  pshufhw_3 =	"rmio:F30F70rMU",
  pshuflw_3 =	"rmio:F20F70rMU",
  pslld_2 =	"rmo:660FF2rM|rio:660F726mU",
  pslldq_2 =	"rio:660F737mU",
  psllq_2 =	"rmo:660FF3rM|rio:660F736mU",
  psllw_2 =	"rmo:660FF1rM|rio:660F716mU",
  psrad_2 =	"rmo:660FE2rM|rio:660F724mU",
  psraw_2 =	"rmo:660FE1rM|rio:660F714mU",
  psrld_2 =	"rmo:660FD2rM|rio:660F722mU",
  psrldq_2 =	"rio:660F733mU",
  psrlq_2 =	"rmo:660FD3rM|rio:660F732mU",
  psrlw_2 =	"rmo:660FD1rM|rio:660F712mU",
  psubb_2 =	"rmo:660FF8rM",
  psubd_2 =	"rmo:660FFArM",
  psubq_2 =	"rmo:660FFBrM",
  psubsb_2 =	"rmo:660FE8rM",
  psubsw_2 =	"rmo:660FE9rM",
  psubusb_2 =	"rmo:660FD8rM",
  psubusw_2 =	"rmo:660FD9rM",
  psubw_2 =	"rmo:660FF9rM",
  punpckhbw_2 =	"rmo:660F68rM",
  punpckhdq_2 =	"rmo:660F6ArM",
  punpckhqdq_2 = "rmo:660F6DrM",
  punpckhwd_2 =	"rmo:660F69rM",
  punpcklbw_2 =	"rmo:660F60rM",
  punpckldq_2 =	"rmo:660F62rM",
  punpcklqdq_2 = "rmo:660F6CrM",
  punpcklwd_2 =	"rmo:660F61rM",
  pxor_2 =	"rmo:660FEFrM",
  rcpps_2 =	"rmo:0F53rM",
  rcpss_2 =	"rro:F30F53rM|rx/od:",
  rsqrtps_2 =	"rmo:0F52rM",
  rsqrtss_2 =	"rmo:F30F52rM",
  sfence_0 =	"0FAEF8",
  shufpd_3 =	"rmio:660FC6rMU",
  shufps_3 =	"rmio:0FC6rMU",
  stmxcsr_1 =   "xd:0FAE3m",
  ucomisd_2 =	"rro:660F2ErM|rx/oq:",
  ucomiss_2 =	"rro:0F2ErM|rx/od:",
  unpckhpd_2 =	"rmo:660F15rM",
  unpckhps_2 =	"rmo:0F15rM",
  unpcklpd_2 =	"rmo:660F14rM",
  unpcklps_2 =	"rmo:0F14rM",
  xorpd_2 =	"rmo:660F57rM",
  xorps_2 =	"rmo:0F57rM",

  -- SSE3 ops
  fisttp_1 =	"xw:nDF1m|xd:DB1m|xq:nDD1m",
  addsubpd_2 =	"rmo:660FD0rM",
  addsubps_2 =	"rmo:F20FD0rM",
  haddpd_2 =	"rmo:660F7CrM",
  haddps_2 =	"rmo:F20F7CrM",
  hsubpd_2 =	"rmo:660F7DrM",
  hsubps_2 =	"rmo:F20F7DrM",
  lddqu_2 =	"rxo:F20FF0rM",
  movddup_2 =	"rmo:F20F12rM",
  movshdup_2 =	"rmo:F30F16rM",
  movsldup_2 =	"rmo:F30F12rM",

  -- SSSE3 ops
  pabsb_2 =	"rmo:660F381CrM",
  pabsd_2 =	"rmo:660F381ErM",
  pabsw_2 =	"rmo:660F381DrM",
  palignr_3 =	"rmio:660F3A0FrMU",
  phaddd_2 =	"rmo:660F3802rM",
  phaddsw_2 =	"rmo:660F3803rM",
  phaddw_2 =	"rmo:660F3801rM",
  phsubd_2 =	"rmo:660F3806rM",
  phsubsw_2 =	"rmo:660F3807rM",
  phsubw_2 =	"rmo:660F3805rM",
  pmaddubsw_2 =	"rmo:660F3804rM",
  pmulhrsw_2 =	"rmo:660F380BrM",
  pshufb_2 =	"rmo:660F3800rM",
  psignb_2 =	"rmo:660F3808rM",
  psignd_2 =	"rmo:660F380ArM",
  psignw_2 =	"rmo:660F3809rM",

  -- SSE4.1 ops
  blendpd_3 =	"rmio:660F3A0DrMU",
  blendps_3 =	"rmio:660F3A0CrMU",
  blendvpd_3 =	"rmRo:660F3815rM",
  blendvps_3 =	"rmRo:660F3814rM",
  dppd_3 =	"rmio:660F3A41rMU",
  dpps_3 =	"rmio:660F3A40rMU",
  extractps_3 =	"mri/do:660F3A17RmU|rri/qo:660F3A17RXmU",
  insertps_3 =	"rrio:660F3A41rMU|rxi/od:",
  movntdqa_2 =	"rxo:660F382ArM",
  mpsadbw_3 =	"rmio:660F3A42rMU",
  packusdw_2 =	"rmo:660F382BrM",
  pblendvb_3 =	"rmRo:660F3810rM",
  pblendw_3 =	"rmio:660F3A0ErMU",
  pcmpeqq_2 =	"rmo:660F3829rM",
  pextrb_3 =	"rri/do:660F3A14nRmU|rri/qo:|xri/bo:",
  pextrd_3 =	"mri/do:660F3A16RmU",
  pextrq_3 =	"mri/qo:660F3A16RmU",
  -- pextrw is SSE2, mem operand is SSE4.1 only
  phminposuw_2 = "rmo:660F3841rM",
  pinsrb_3 =	"rri/od:660F3A20nrMU|rxi/ob:",
  pinsrd_3 =	"rmi/od:660F3A22rMU",
  pinsrq_3 =	"rmi/oq:660F3A22rXMU",
  pmaxsb_2 =	"rmo:660F383CrM",
  pmaxsd_2 =	"rmo:660F383DrM",
  pmaxud_2 =	"rmo:660F383FrM",
  pmaxuw_2 =	"rmo:660F383ErM",
  pminsb_2 =	"rmo:660F3838rM",
  pminsd_2 =	"rmo:660F3839rM",
  pminud_2 =	"rmo:660F383BrM",
  pminuw_2 =	"rmo:660F383ArM",
  pmovsxbd_2 =	"rro:660F3821rM|rx/od:",
  pmovsxbq_2 =	"rro:660F3822rM|rx/ow:",
  pmovsxbw_2 =	"rro:660F3820rM|rx/oq:",
  pmovsxdq_2 =	"rro:660F3825rM|rx/oq:",
  pmovsxwd_2 =	"rro:660F3823rM|rx/oq:",
  pmovsxwq_2 =	"rro:660F3824rM|rx/od:",
  pmovzxbd_2 =	"rro:660F3831rM|rx/od:",
  pmovzxbq_2 =	"rro:660F3832rM|rx/ow:",
  pmovzxbw_2 =	"rro:660F3830rM|rx/oq:",
  pmovzxdq_2 =	"rro:660F3835rM|rx/oq:",
  pmovzxwd_2 =	"rro:660F3833rM|rx/oq:",
  pmovzxwq_2 =	"rro:660F3834rM|rx/od:",
  pmuldq_2 =	"rmo:660F3828rM",
  pmulld_2 =	"rmo:660F3840rM",
  ptest_2 =	"rmo:660F3817rM",
  roundpd_3 =	"rmio:660F3A09rMU",
  roundps_3 =	"rmio:660F3A08rMU",
  roundsd_3 =	"rrio:660F3A0BrMU|rxi/oq:",
  roundss_3 =	"rrio:660F3A0ArMU|rxi/od:",

  -- SSE4.2 ops
  crc32_2 =	"rmqd:F20F38F1rM|rm/dw:66F20F38F1rM|rm/db:F20F38F0rM|rm/qb:",
  pcmpestri_3 =	"rmio:660F3A61rMU",
  pcmpestrm_3 =	"rmio:660F3A60rMU",
  pcmpgtq_2 =	"rmo:660F3837rM",
  pcmpistri_3 =	"rmio:660F3A63rMU",
  pcmpistrm_3 =	"rmio:660F3A62rMU",
  popcnt_2 =	"rmqdw:F30FB8rM",

  -- SSE4a
  extrq_2 =	"rro:660F79rM",
  extrq_3 =	"riio:660F780mUU",
  insertq_2 =	"rro:F20F79rM",
  insertq_4 =	"rriio:F20F78rMUU",
  lzcnt_2 =	"rmqdw:F30FBDrM",
  movntsd_2 =	"xr/qo:nF20F2BRm",
  movntss_2 =	"xr/do:F30F2BRm",
  -- popcnt is also in SSE4.2
}
```