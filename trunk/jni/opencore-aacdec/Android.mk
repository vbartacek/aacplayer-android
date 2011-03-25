LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := opencore-aacdec
LOCAL_SRC_FILES := src/sbr_create_limiter_bands.c src/get_ga_specific_config.c src/idct32.c \
				src/tns_decode_coef.c src/digit_reversal_tables.c src/sbr_decode_huff_cw.c \
				src/sbr_inv_filt_levelemphasis.c src/pvmp4audiodecoderinitlibrary.c src/intensity_right.c \
				src/ps_all_pass_fract_delay_filter.c src/fwd_short_complex_rot.c src/pns_intensity_right.c \
				src/mix_radix_fft.c src/tns_ar_filter.c src/get_dse.c \
				src/inv_long_complex_rot.c src/q_normalize.c src/dct64.c \
				src/decode_noise_floorlevels.c src/hcbtables_binary.c src/extractframeinfo.c \
				src/check_crc.c src/esc_iquant_scaling.c src/getmask.c \
				src/sbr_get_additional_data.c src/get_adif_header.c src/pv_div.c \
				src/pvmp4setaudioconfig.c src/sbr_crc_check.c src/qmf_filterbank_coeff.c \
				src/getfill.c src/sbr_get_dir_control_data.c src/getactualaacconfig.c \
				src/sbr_extract_extended_data.c src/getics.c src/apply_ms_synt.c \
				src/pulse_nc.c src/sbr_requantize_envelope_data.c src/sbr_open.c \
				src/decode_huff_cw_binary.c src/gen_rand_vector.c src/calc_sbr_envelope.c \
				src/fft_rx4_short.c src/iquant_table.c src/long_term_synthesis.c \
				src/buf_getbits.c src/ps_decode_bs_utils.c src/deinterleave.c \
				src/sbr_get_envelope.c src/dst32.c src/get_pulse_data.c \
				src/sbr_get_cpe.c src/calc_sbr_synfilterbank.c src/fft_rx4_long.c \
				src/pv_sine.c src/pns_left.c src/ps_hybrid_filter_bank_allocation.c \
				src/window_tables_fxp.c src/get_tns.c src/dst16.c \
				src/sbr_downsample_lo_res.c src/synthesis_sub_band.c src/calc_auto_corr.c \
				src/long_term_prediction.c src/ms_synt.c src/inv_short_complex_rot.c \
				src/hufffac.c src/sbr_code_book_envlevel.c src/get_sbr_bitstream.c \
				src/get_adts_header.c src/sfb.c src/sbr_update_freq_scale.c \
				src/sbr_generate_high_freq.c src/sbr_get_sce.c src/huffdecode.c \
				src/pv_sqrt.c src/tns_inv_filter.c src/ps_hybrid_analysis.c \
				src/sbr_dec.c src/sbr_envelope_calc_tbl.c src/analysis_sub_band.c \
				src/ps_decorrelate.c src/pv_normalize.c src/dst8.c \
				src/find_adts_syncword.c src/ps_read_data.c src/pvmp4audiodecodergetmemrequirements.c \
				src/sbr_decode_envelope.c src/ps_init_stereo_mixing.c src/pv_pow2.c \
				src/fwd_long_complex_rot.c src/sbr_find_start_andstop_band.c src/lt_decode.c \
				src/get_ele_list.c src/unpack_idx.c src/get_sbr_startfreq.c \
				src/ps_hybrid_synthesis.c src/sbr_read_data.c src/ps_channel_filtering.c \
				src/mdct_fxp.c src/sbr_aliasing_reduction.c src/imdct_fxp.c \
				src/get_ics_info.c src/mdct_tables_fxp.c src/ps_stereo_processing.c \
				src/get_prog_config.c src/fft_rx4_tables_fxp.c src/trans4m_freq_2_time_fxp.c \
				src/apply_tns.c src/getgroup.c src/set_mc_info.c \
				src/sbr_reset_dec.c src/pvmp4audiodecoderresetbuffer.c src/ps_allocate_decoder.c \
				src/pvmp4audiodecoderframe.c src/pv_log2.c src/init_sbr_dec.c \
				src/infoinit.c src/mdst.c src/ps_pwr_transient_detection.c \
				src/sbr_get_header_data.c src/pns_corr.c src/huffcb.c \
				src/calc_sbr_anafilterbank.c src/dct16.c src/trans4m_time_2_freq_fxp.c \
				src/sbr_envelope_unmapping.c src/ps_bstr_decoding.c src/idct8.c \
				src/calc_gsfb_table.c src/get_audio_specific_config.c src/huffspec_fxp.c \
				src/ps_fft_rx8.c src/get_sbr_stopfreq.c src/sbr_applied.c \
				src/pvmp4audiodecoderconfig.c src/ps_applied.c src/ps_all_pass_filter_coeff.c \
				src/byte_align.c src/idct16.c src/sbr_get_noise_floor_data.c \
				src/shellsort.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include $(LOCAL_PATH)/src $(LOCAL_PATH)/oscl
LOCAL_ARM_MODE := arm

include $(BUILD_STATIC_LIBRARY)
