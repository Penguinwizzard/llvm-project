//===-- PPCInstrPaired.td - The PowerPC Paired Single Extension -*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file describes the Paired Single extension to the PowerPC instruction set.
//
//===----------------------------------------------------------------------===//

multiclass PSForm_xdabr<bits<10> xo, dag OOL, dag IOL, string asmbase,
                        string asmstr, InstrItinClass itin, list<dag> pattern> {
  let BaseName = asmbase in {
    def NAME : PSForm_xdab<xo, OOL, IOL,
                           !strconcat(asmbase, !strconcat(" ", asmstr)), itin,
                           pattern>, RecFormRel;
    let Defs = [CR1] in
    def _rec : PSForm_xdab<xo, OOL, IOL,
                           !strconcat(asmbase, !strconcat(". ", asmstr)), itin,
                           []>, isRecordForm, RecFormRel;
  }
}

multiclass PSForm_xd0br<bits<10> xo, dag OOL, dag IOL, string asmbase,
                        string asmstr, InstrItinClass itin, list<dag> pattern> {
  let BaseName = asmbase in {
    def NAME : PSForm_xd0b<xo, OOL, IOL,
                           !strconcat(asmbase, !strconcat(" ", asmstr)), itin,
                           pattern>, RecFormRel;
    let Defs = [CR1] in
    def _rec : PSForm_xd0b<xo, OOL, IOL,
                           !strconcat(asmbase, !strconcat(". ", asmstr)), itin,
                           []>, isRecordForm, RecFormRel;
  }
}

multiclass PSForm_adabcr<bits<5> xo, dag OOL, dag IOL, string asmbase,
                        string asmstr, InstrItinClass itin, list<dag> pattern> {
  let BaseName = asmbase in {
    def NAME : PSForm_adabc<xo, OOL, IOL,
                            !strconcat(asmbase, !strconcat(" ", asmstr)), itin,
                            pattern>, RecFormRel;
    let Defs = [CR1] in
    def _rec : PSForm_adabc<xo, OOL, IOL,
                            !strconcat(asmbase, !strconcat(". ", asmstr)), itin,
                            []>, isRecordForm, RecFormRel;
  }
}

multiclass PSForm_ada0cr<bits<5> xo, dag OOL, dag IOL, string asmbase,
                        string asmstr, InstrItinClass itin, list<dag> pattern> {
  let BaseName = asmbase in {
    def NAME : PSForm_ada0c<xo, OOL, IOL,
                            !strconcat(asmbase, !strconcat(" ", asmstr)), itin,
                            pattern>, RecFormRel;
    let Defs = [CR1] in
    def _rec : PSForm_ada0c<xo, OOL, IOL,
                            !strconcat(asmbase, !strconcat(". ", asmstr)), itin,
                            []>, isRecordForm, RecFormRel;
  }
}

multiclass PSForm_adab0r<bits<5> xo, dag OOL, dag IOL, string asmbase,
                        string asmstr, InstrItinClass itin, list<dag> pattern> {
  let BaseName = asmbase in {
    def NAME : PSForm_adab0<xo, OOL, IOL,
                            !strconcat(asmbase, !strconcat(" ", asmstr)), itin,
                            pattern>, RecFormRel;
    let Defs = [CR1] in
    def _rec : PSForm_adab0<xo, OOL, IOL,
                            !strconcat(asmbase, !strconcat(". ", asmstr)), itin,
                            []>, isRecordForm, RecFormRel;
  }
}

multiclass PSForm_ad0b0r<bits<5> xo, dag OOL, dag IOL, string asmbase,
                        string asmstr, InstrItinClass itin, list<dag> pattern> {
  let BaseName = asmbase in {
    def NAME : PSForm_ad0b0<xo, OOL, IOL,
                            !strconcat(asmbase, !strconcat(" ", asmstr)), itin,
                            pattern>, RecFormRel;
    let Defs = [CR1] in
    def _rec : PSForm_ad0b0<xo, OOL, IOL,
                            !strconcat(asmbase, !strconcat(". ", asmstr)), itin,
                            []>, isRecordForm, RecFormRel;
  }
}

let DecoderNamespace = "PairedSingle" in {
    let Predicates = [HasPaired] in {
        def DCBZ_L : PSForm_x0ab<1014, (outs), (ins memrr:$dst), "dcbz_l $dst",
                                IIC_LdStDCBF, [(int_ppc_broadway_dcbz_l xoaddr:$dst)]>;

        def PSQ_L : PSQForm_dw<56, (outs psrc:$fD), (ins memrid12:$src, u1imm:$W, u3imm:$I),
                              "psq_l $fD, $src, $W, $I", IIC_LdStLoad, []>;

        def PSQ_LU : PSQForm_dw<57, (outs psrc:$fD, ptr_rc_nor0:$ea_result),
                                (ins memrid12:$src, u1imm:$W, u3imm:$I),
                                "psq_lu $fD, $src, $W, $I", IIC_LdStLoadUpd, []>,
                                RegConstraint<"$src.reg = $ea_result">,
                                NoEncode<"$ea_result">;
        def PSQ_LX : PSQForm_xw<6, (outs psrc:$fD), (ins memrr:$src, u1imm:$W, u3imm:$I),
                                "psq_lx $fD, $src, $W, $I", IIC_LdStLoad, []>;
        def PSQ_LUX : PSQForm_xw<38, (outs psrc:$fD, ptr_rc_nor0:$ea_result),
                                (ins memrr:$src, u1imm:$W, u3imm:$I),
                                "psq_lux $fD, $src, $W, $I", IIC_LdStLoad, []>,
                                RegConstraint<"$src.ptrreg = $ea_result">,
                                NoEncode<"$ea_result">;

        def PSQ_ST : PSQForm_dw<60, (outs), (ins psrc:$fD, memrid12:$dst, u1imm:$W, u3imm:$I),
                                "psq_st $fD, $dst, $W, $I", IIC_LdStStore, []>;

        def PSQ_STU : PSQForm_dw<61, (outs ptr_rc_nor0:$ea_result),
                                (ins psrc:$fD, memrid12:$dst, u1imm:$W, u3imm:$I),
                                "psq_stu $fD, $dst, $W, $I", IIC_LdStSTU, []>,
                                RegConstraint<"$dst.reg = $ea_result">,
                                NoEncode<"$ea_result">;

        def PSQ_STX : PSQForm_xw<7, (outs), (ins psrc:$fD, memrr:$dst, u1imm:$W, u3imm:$I),
                                "psq_stx $fD, $dst, $W, $I", IIC_LdStStore, []>;

        def PSQ_STUX : PSQForm_xw<39, (outs ptr_rc_nor0:$ea_result),
                                  (ins psrc:$fD, memrr:$dst, u1imm:$W, u3imm:$I),
                                  "psq_stux $fD, $dst, $W, $I", IIC_LdStSTUX, []>,
                                  RegConstraint<"$dst.ptrreg = $ea_result">,
                                  NoEncode<"$ea_result">;

        defm PS_ABS : PSForm_xd0br<264, (outs psrc:$fD), (ins psrc:$fB),
                                    "ps_abs", "$fD, $fB", IIC_FPGeneral,
                                    [(set v2f32:$fD, (fabs v2f32:$fB))]>;

        defm PS_ADD : PSForm_adab0r<21, (outs psrc:$fD), (ins psrc:$fA, psrc:$fB),
                                    "ps_add", "$fD, $fA, $fB", IIC_FPAddSub,
                                    [(set v2f32:$fD, (fadd v2f32:$fA, v2f32:$fB))]>;

        def PS_CMPU0 : PSForm_xcrab<0, 0, (outs crrc:$crD), (ins psrc:$fA, psrc:$fB),
                                    "ps_cmpu0 $crD, $fA, $fB", IIC_FPCompare, []>;
        def PS_CMPU1 : PSForm_xcrab<0, 1, (outs crrc:$crD), (ins psrc:$fA, psrc:$fB),
                                    "ps_cmpu1 $crD, $fA, $fB", IIC_FPCompare, []>;
        def PS_CMPO0 : PSForm_xcrab<1, 0, (outs crrc:$crD), (ins psrc:$fA, psrc:$fB),
                                    "ps_cmpo0 $crD, $fA, $fB", IIC_FPCompare, []>;
        def PS_CMPO1 : PSForm_xcrab<1, 1, (outs crrc:$crD), (ins psrc:$fA, psrc:$fB),
                                    "ps_cmpo1 $crD, $fA, $fB", IIC_FPCompare, []>;

        defm PS_DIV : PSForm_adab0r<18, (outs psrc:$fD), (ins psrc:$fA, psrc:$fB),
                                    "ps_div", "$fD, $fA, $fB", IIC_FPDivS,
                                    [(set v2f32:$fD, (fdiv v2f32:$fA, v2f32:$fB))]>;

        defm PS_MADD : PSForm_adabcr<29, (outs psrc:$fD), (ins psrc:$fA, psrc:$fB, psrc:$fC),
                                    "ps_madd", "$fD, $fA, $fC, $fB", IIC_FPFused,
                                    [(set v2f32:$fD, (fadd (fmul v2f32:$fA, v2f32:$fC), v2f32:$fB))]>;

        defm PS_MADDS0 : PSForm_adabcr<14, (outs psrc:$fD), (ins psrc:$fA, psrc:$fB, psrc:$fC),
                                      "ps_madds0", "$fD, $fA, $fC, $fB", IIC_FPFused,
                                      []>;

        defm PS_MADDS1 : PSForm_adabcr<15, (outs psrc:$fD), (ins psrc:$fA, psrc:$fB, psrc:$fC),
                                      "ps_madds1", "$fD, $fA, $fC, $fB", IIC_FPFused,
                                      []>;
        defm PS_MERGE00 : PSForm_xdabr<528, (outs psrc:$fD), (ins psrc:$fA, psrc:$fB),
                                      "ps_merge00", "$fD, $fA, $fB", IIC_FPGeneral,
                                      []>;
        defm PS_MERGE01 : PSForm_xdabr<560, (outs psrc:$fD), (ins psrc:$fA, psrc:$fB),
                                      "ps_merge01", "$fD, $fA, $fB", IIC_FPGeneral,
                                      []>;
        defm PS_MERGE10 : PSForm_xdabr<592, (outs psrc:$fD), (ins psrc:$fA, psrc:$fB),
                                      "ps_merge10", "$fD, $fA, $fB", IIC_FPGeneral,
                                      []>;
        defm PS_MERGE11 : PSForm_xdabr<624, (outs psrc:$fD), (ins psrc:$fA, psrc:$fB),
                                      "ps_merge11", "$fD, $fA, $fB", IIC_FPGeneral,
                                      []>;
        defm PS_MR : PSForm_xd0br<72, (outs psrc:$fD), (ins psrc:$fB),
                                      "ps_mr", "$fD, $fB", IIC_FPGeneral,
                                      [(set v2f32:$fD, v2f32:$fB)]>;

        defm PS_MSUB : PSForm_adabcr<28, (outs psrc:$fD), (ins psrc:$fA, psrc:$fB, psrc:$fC),
                                    "ps_msub", "$fD, $fA, $fC, $fB", IIC_FPFused,
                                    [(set v2f32:$fD, (fsub (fmul v2f32:$fA, v2f32:$fC), v2f32:$fB))]>;

        defm PS_MUL : PSForm_ada0cr<25, (outs psrc:$fD), (ins psrc:$fA, psrc:$fC),
                                    "ps_mul", "$fD, $fA, $fC", IIC_FPFused,
                                    [(set v2f32:$fD, (fmul v2f32:$fA, v2f32:$fC))]>;

        defm PS_MULS0 : PSForm_ada0cr<12, (outs psrc:$fD), (ins psrc:$fA, psrc:$fC),
                                    "ps_muls0", "$fD, $fA, $fC", IIC_FPFused, []>;
        defm PS_MULS1 : PSForm_ada0cr<13, (outs psrc:$fD), (ins psrc:$fA, psrc:$fC),
                                    "ps_muls1", "$fD, $fA, $fC", IIC_FPFused, []>;

        defm PS_NABS : PSForm_xd0br<136, (outs psrc:$fD), (ins psrc:$fB),
                                    "ps_nabs", "$fD, $fB", IIC_FPGeneral,
                                    [(set v2f32:$fD, (fneg (fabs v2f32:$fB)))]>;

        defm PS_NEG : PSForm_xd0br<40, (outs psrc:$fD), (ins psrc:$fB),
                                    "ps_neg", "$fD, $fB", IIC_FPGeneral,
                                    [(set v2f32:$fD, (fneg v2f32:$fB))]>;

        defm PS_NMADD : PSForm_adabcr<31, (outs psrc:$fD), (ins psrc:$fA, psrc:$fB, psrc:$fC),
                                    "ps_nmadd", "$fD, $fA, $fC, $fB", IIC_FPFused,
                                    [(set v2f32:$fD, (fneg (fadd (fmul v2f32:$fA, v2f32:$fC), v2f32:$fB)))]>;

       defm PS_NMSUB : PSForm_adabcr<30, (outs psrc:$fD), (ins psrc:$fA, psrc:$fB, psrc:$fC),
                                    "ps_nmsub", "$fD, $fA, $fC, $fB", IIC_FPFused,
                                    [(set v2f32:$fD, (fneg (fsub (fmul v2f32:$fA, v2f32:$fC), v2f32:$fB)))]>;

        defm PS_RES : PSForm_ad0b0r<24, (outs psrc:$fD), (ins psrc:$fB),
                                    "ps_res", "$fD, $fB", IIC_FPGeneral,
                                    [(set v2f32:$fD, (PPCfre v2f32:$fB))]>;

        defm PS_RSQRTE : PSForm_ad0b0r<26, (outs psrc:$fD), (ins psrc:$fB),
                                      "ps_rsqrte", "$fD, $fB", IIC_FPGeneral,
                                      [(set v2f32:$fD, (PPCfrsqrte v2f32:$fB))]>;

      defm PS_SEL : PSForm_adabcr<23, (outs psrc:$fD), (ins psrc:$fA, psrc:$fB, psrc:$fC),
                                  "ps_sel", "$fD, $fA, $fC, $fB", IIC_FPGeneral,
                                  []>;

        defm PS_SUB : PSForm_adab0r<20, (outs psrc:$fD), (ins psrc:$fA, psrc:$fB),
                                    "ps_sub", "$fD, $fA, $fB", IIC_FPAddSub,
                                    [(set v2f32:$fD, (fsub v2f32:$fA, v2f32:$fB))]>;

       defm PS_SUM0 : PSForm_adabcr<10, (outs psrc:$fD), (ins psrc:$fA, psrc:$fB, psrc:$fC),
                                    "ps_sum0", "$fD, $fA, $fC, $fB", IIC_FPAddSub,
                                    []>;
      defm PS_SUM1 : PSForm_adabcr<11, (outs psrc:$fD), (ins psrc:$fA, psrc:$fB, psrc:$fC),
                                    "ps_sum1", "$fD, $fA, $fC, $fB", IIC_FPAddSub,
                                    []>;
    }
}