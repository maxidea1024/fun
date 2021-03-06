#define mp_int	pn_mp_int
#define ltm_prime_callback fun_ltm_prime_callback
#define mp_error_to_string fun_mp_error_to_string
#define mp_init fun_mp_init
#define mp_clear fun_mp_clear
#define mp_init_multi fun_mp_init_multi
#define mp_clear_multi fun_mp_clear_multi
#define mp_exch fun_mp_exch
#define mp_shrink fun_mp_shrink
#define mp_grow fun_mp_grow
#define mp_init_size fun_mp_init_size
#define mp_zero fun_mp_zero
#define mp_set fun_mp_set
#define mp_set_int fun_mp_set_int
#define mp_get_int fun_mp_get_int
#define mp_init_set  fun_mp_init_set 
#define mp_init_set_int  fun_mp_init_set_int 
#define mp_copy fun_mp_copy
#define mp_init_copy fun_mp_init_copy
#define mp_clamp fun_mp_clamp
#define mp_rshd fun_mp_rshd
#define mp_lshd fun_mp_lshd
#define mp_div_2d fun_mp_div_2d
#define mp_div_2 fun_mp_div_2
#define mp_mul_2d fun_mp_mul_2d
#define mp_mul_2 fun_mp_mul_2
#define mp_mod_2d fun_mp_mod_2d
#define mp_2expt fun_mp_2expt
#define mp_cnt_lsb fun_mp_cnt_lsb
#define mp_rand fun_mp_rand
#define mp_xor fun_mp_xor
#define mp_or fun_mp_or
#define mp_and fun_mp_and
#define mp_neg fun_mp_neg
#define mp_abs fun_mp_abs
#define mp_cmp fun_mp_cmp
#define mp_cmp_mag fun_mp_cmp_mag
#define mp_add fun_mp_add
#define mp_sub fun_mp_sub
#define mp_mul fun_mp_mul
#define mp_sqr fun_mp_sqr
#define mp_div fun_mp_div
#define mp_mod fun_mp_mod
#define mp_cmp_d fun_mp_cmp_d
#define mp_add_d fun_mp_add_d
#define mp_sub_d fun_mp_sub_d
#define mp_mul_d fun_mp_mul_d
#define mp_div_d fun_mp_div_d
#define mp_div_3 fun_mp_div_3
#define mp_expt_d fun_mp_expt_d
#define mp_mod_d fun_mp_mod_d
#define mp_addmod fun_mp_addmod
#define mp_submod fun_mp_submod
#define mp_mulmod fun_mp_mulmod
#define mp_sqrmod fun_mp_sqrmod
#define mp_invmod fun_mp_invmod
#define mp_gcd fun_mp_gcd
#define mp_exteuclid fun_mp_exteuclid
#define mp_lcm fun_mp_lcm
#define mp_n_root fun_mp_n_root
#define mp_sqrt fun_mp_sqrt
#define mp_is_square fun_mp_is_square
#define mp_jacobi fun_mp_jacobi
#define mp_reduce_setup fun_mp_reduce_setup
#define mp_reduce fun_mp_reduce
#define mp_montgomery_setup fun_mp_montgomery_setup
#define mp_montgomery_calc_normalization fun_mp_montgomery_calc_normalization
#define mp_montgomery_reduce fun_mp_montgomery_reduce
#define mp_dr_is_modulus fun_mp_dr_is_modulus
#define mp_dr_setup fun_mp_dr_setup
#define mp_dr_reduce fun_mp_dr_reduce
#define mp_reduce_is_2k fun_mp_reduce_is_2k
#define mp_reduce_2k_setup fun_mp_reduce_2k_setup
#define mp_reduce_2k fun_mp_reduce_2k
#define mp_reduce_is_2k_l fun_mp_reduce_is_2k_l
#define mp_reduce_2k_setup_l fun_mp_reduce_2k_setup_l
#define mp_reduce_2k_l fun_mp_reduce_2k_l
#define mp_exptmod fun_mp_exptmod
#define mp_prime_is_divisible fun_mp_prime_is_divisible
#define mp_prime_fermat fun_mp_prime_fermat
#define mp_prime_miller_rabin fun_mp_prime_miller_rabin
#define mp_prime_rabin_miller_trials fun_mp_prime_rabin_miller_trials
#define mp_prime_is_prime fun_mp_prime_is_prime
#define mp_prime_next_prime fun_mp_prime_next_prime
#define mp_prime_random_ex fun_mp_prime_random_ex
#define mp_count_bits fun_mp_count_bits
#define mp_unsigned_bin_size fun_mp_unsigned_bin_size
#define mp_read_unsigned_bin fun_mp_read_unsigned_bin
#define mp_to_unsigned_bin fun_mp_to_unsigned_bin
#define mp_to_unsigned_bin_n  fun_mp_to_unsigned_bin_n 
#define mp_signed_bin_size fun_mp_signed_bin_size
#define mp_read_signed_bin fun_mp_read_signed_bin
#define mp_to_signed_bin fun_mp_to_signed_bin
#define mp_to_signed_bin_n  fun_mp_to_signed_bin_n 
#define mp_read_radix fun_mp_read_radix
#define mp_toradix fun_mp_toradix
#define mp_toradix_n fun_mp_toradix_n
#define mp_radix_size fun_mp_radix_size
#define mp_fread fun_mp_fread
#define mp_fwrite fun_mp_fwrite
#define s_mp_add fun_s_mp_add
#define s_mp_sub fun_s_mp_sub
#define fast_s_mp_mul_digs fun_fast_s_mp_mul_digs
#define s_mp_mul_digs fun_s_mp_mul_digs
#define fast_s_mp_mul_high_digs fun_fast_s_mp_mul_high_digs
#define s_mp_mul_high_digs fun_s_mp_mul_high_digs
#define fast_s_mp_sqr fun_fast_s_mp_sqr
#define s_mp_sqr fun_s_mp_sqr
#define mp_karatsuba_mul fun_mp_karatsuba_mul
#define mp_toom_mul fun_mp_toom_mul
#define mp_karatsuba_sqr fun_mp_karatsuba_sqr
#define mp_toom_sqr fun_mp_toom_sqr
#define fast_mp_invmod fun_fast_mp_invmod
#define mp_invmod_slow  fun_mp_invmod_slow 
#define fast_mp_montgomery_reduce fun_fast_mp_montgomery_reduce
#define mp_exptmod_fast fun_mp_exptmod_fast
#define s_mp_exptmod  fun_s_mp_exptmod 
#define bn_reverse fun_bn_reverse
#define mp_s_rmap fun_mp_s_rmap
#define KARATSUBA_MUL_CUTOFF fun_KARATSUBA_MUL_CUTOFF
#define KARATSUBA_SQR_CUTOFF fun_KARATSUBA_SQR_CUTOFF
#define TOOM_MUL_CUTOFF fun_TOOM_MUL_CUTOFF
#define TOOM_SQR_CUTOFF fun_TOOM_SQR_CUTOFF
#define ltm_prime_tab fun_ltm_prime_tab
