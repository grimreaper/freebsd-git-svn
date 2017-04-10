 * Copyright (c) 2016-2017 Robert N. M. Watson
 * Portions of this software were developed by BAE Systems, the University of
 * Cambridge Computer Laboratory, and Memorial University under DARPA/AFRL
 * contract FA8650-15-C-7558 ("CADETS"), as part of the DARPA Transparent
 * Computing (TC) research program.
 *
	au_evnamemap_init();
		if (ARG_IS_VALID(kar, ARG_FD)) {
			tok = au_to_arg32(1, "fd", ar->ar_arg_fd);
			kau_write(rec, tok);
		}
		if (ARG_IS_VALID(kar, ARG_SADDRINET)) {
			tok = au_to_sock_inet((struct sockaddr_in *)
			    &ar->ar_arg_sockaddr);
			kau_write(rec, tok);
		}
		if (ARG_IS_VALID(kar, ARG_SADDRUNIX)) {
			tok = au_to_sock_unix((struct sockaddr_un *)
			    &ar->ar_arg_sockaddr);
			kau_write(rec, tok);
			UPATH1_TOKENS;
		}
		break;

	case AUE_SENDFILE:
		FD_VNODE1_TOKENS;
		if (ARG_IS_VALID(kar, ARG_SADDRINET)) {
			tok = au_to_sock_inet((struct sockaddr_in *)
			    &ar->ar_arg_sockaddr);
			kau_write(rec, tok);
		}
		if (ARG_IS_VALID(kar, ARG_SADDRUNIX)) {
			tok = au_to_sock_unix((struct sockaddr_un *)
			    &ar->ar_arg_sockaddr);
			kau_write(rec, tok);
			UPATH1_TOKENS;
		}
		/* XXX Need to handle ARG_SADDRINET6 */
		break;

	case AUE_ACL_DELETE_FD:
	case AUE_ACL_DELETE_FILE:
	case AUE_ACL_CHECK_FD:
	case AUE_ACL_CHECK_FILE:
	case AUE_ACL_CHECK_LINK:
	case AUE_ACL_DELETE_LINK:
	case AUE_ACL_GET_FD:
	case AUE_ACL_GET_FILE:
	case AUE_ACL_GET_LINK:
	case AUE_ACL_SET_FD:
	case AUE_ACL_SET_FILE:
	case AUE_ACL_SET_LINK:
		if (ARG_IS_VALID(kar, ARG_VALUE)) {
			tok = au_to_arg32(1, "type", ar->ar_arg_value);
			kau_write(rec, tok);
		}
		ATFD1_TOKENS(1);
		UPATH1_VNODE1_TOKENS;
		break;

	case AUE_POSIX_FALLOCATE:
	case AUE_PROCCTL:
		if (ARG_IS_VALID(kar, ARG_VALUE)) {
			tok = au_to_arg32(1, "idtype", ar->ar_arg_value);
			kau_write(rec, tok);
		}
		if (ARG_IS_VALID(kar, ARG_CMD)) {
			tok = au_to_arg32(2, "com", ar->ar_arg_cmd);
			kau_write(rec, tok);
		}
		PROCESS_PID_TOKENS(3);
		break;

		if (ARG_IS_VALID(kar, ARG_FFLAGS)) {
		UPATH1_TOKENS;