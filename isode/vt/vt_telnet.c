/*****************************************************************************

 @(#) $RCSfile$ $Name$($Revision$) $Date$

 -----------------------------------------------------------------------------

 Copyright (c) 2001-2007  OpenSS7 Corporation <http://www.openss7.com/>
 Copyright (c) 1997-2000  Brian F. G. Bidulock <bidulock@openss7.org>

 All Rights Reserved.

 This program is free software: you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation, version 3 of the license.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 details.

 You should have received a copy of the GNU General Public License along with
 this program.  If not, see <http://www.gnu.org/licenses/>, or write to the
 Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

 -----------------------------------------------------------------------------

 U.S. GOVERNMENT RESTRICTED RIGHTS.  If you are licensing this Software on
 behalf of the U.S. Government ("Government"), the following provisions apply
 to you.  If the Software is supplied by the Department of Defense ("DoD"), it
 is classified as "Commercial Computer Software" under paragraph 252.227-7014
 of the DoD Supplement to the Federal Acquisition Regulations ("DFARS") (or any
 successor regulations) and the Government is acquiring only the license rights
 granted herein (the license rights customarily provided to non-Government
 users).  If the Software is supplied to any unit or agency of the Government
 other than DoD, it is classified as "Restricted Computer Software" and the
 Government's rights in the Software are defined in paragraph 52.227-19 of the
 Federal Acquisition Regulations ("FAR") (or any successor regulations) or, in
 the cases of NASA, in paragraph 18.52.227-86 of the NASA Supplement to the FAR
 (or any successor regulations).

 -----------------------------------------------------------------------------

 Commercial licensing and support of this software is available from OpenSS7
 Corporation at a fee.  See http://www.openss7.com/

 -----------------------------------------------------------------------------

 Last Modified $Date$ by $Author$

 -----------------------------------------------------------------------------

 $Log$
 *****************************************************************************/

#ident "@(#) $RCSfile$ $Name$($Revision$) $Date$"

static char const ident[] = "$RCSfile$ $Name$($Revision$) $Date$";

/* vt_telnet.c - VT telnet profile */

#ifndef	lint
static char *rcsid =
    "Header: /xtel/isode/isode/vt/RCS/vt_telnet.c,v 9.0 1992/06/16 12:41:08 isode Rel";
#endif

/* 
 * Header: /xtel/isode/isode/vt/RCS/vt_telnet.c,v 9.0 1992/06/16 12:41:08 isode Rel
 *
 *
 * Log: vt_telnet.c,v
 * Revision 9.0  1992/06/16  12:41:08  isode
 * Release 8.0
 *
 */

/*
 *				  NOTICE
 *
 *    Acquisition, use, and distribution of this module and related
 *    materials are subject to the restrictions of a license agreement.
 *    Consult the Preface in the User's Manual for the full terms of
 *    this agreement.
 *
 */

#include "vtpm.h"
#include "sector1.h"
#ifdef SVR4_UCB
#include <termio.h>
#else
#include <sys/ioctl.h>
#endif

#define VT_BREAK
#undef PEPYPARM
#define PEPYPARM int *

extern struct sgttyb ottyb;
extern int cur_emode;
extern char *my_displayobj;
extern char *my_signal_obj;
extern char *my_echo_obj;
extern char kb_image;
extern char di_image;
extern char ni_image;
extern char na_image;
extern char sync_image;
extern char ga_image;
extern char nego_state;
extern int my_right;
extern transparent;
extern do_break;
extern telnet_profile;
extern int connected;

vt_newline()
{				/* Produce Newline update */

	TEXT_UPDATE ud;

	bzero((char *) &ud, sizeof ud);
	ud.echo_sw = cur_emode;
	ud.type_sw = DISPLAY_OBJ;
	ud.updates.do_list.do_name = my_displayobj;
	ud.updates.do_list.do_type = DO_NEXT_X;	/* Next X-Array */
	send_queue(ud);
}

vt_char_erase()
{				/* Pointer Relative (x=x-1) & erase current */

	TEXT_UPDATE ud;

	bzero((char *) &ud, sizeof ud);
	ud.echo_sw = cur_emode;
	ud.type_sw = DISPLAY_OBJ;
	ud.updates.do_list.do_name = my_displayobj;
	ud.updates.do_list.do_type = DO_PTR_REL;
	ud.updates.do_list.do_cmd.ptr_rel.x_true = 1;
	ud.updates.do_list.do_cmd.ptr_rel.y_true = 0;
	ud.updates.do_list.do_cmd.ptr_rel.z_true = 0;
	ud.updates.do_list.do_cmd.ptr_rel.x_value = -1;

	send_queue(ud);

	ud.updates.do_list.do_type = DO_ERASE;
	ud.updates.do_list.do_cmd.erase.start_erase.ptr_type = 0;	/* Current */
	ud.updates.do_list.do_cmd.erase.end_erase.ptr_type = 0;	/* Current */
	ud.updates.do_list.do_cmd.erase.erase_attr = 0;

	send_queue(ud);
}

vt_line_erase()
{				/* Erase full x-array & pointer to x = 1 */

	TEXT_UPDATE ud;

	bzero((char *) &ud, sizeof ud);
	ud.echo_sw = cur_emode;
	ud.type_sw = DISPLAY_OBJ;
	ud.updates.do_list.do_name = my_displayobj;
	ud.updates.do_list.do_type = DO_ERASE;
	ud.updates.do_list.do_cmd.erase.start_erase.ptr_type = 3;	/* Start X */
	ud.updates.do_list.do_cmd.erase.end_erase.ptr_type = 6;	/* End X */
	ud.updates.do_list.do_cmd.erase.erase_attr = 0;

	send_queue(ud);

	ud.updates.do_list.do_type = DO_PTR_ABS;
	ud.updates.do_list.do_cmd.ptr_abs.ptr_type = 3;	/* Start X */

	send_queue(ud);
}

vt_interrupt()
{				/* Toggle Bit 1 of DI/KB control object */

	TEXT_UPDATE ud;
	char int_mask;
	char image;

	int_mask = IP_OBJ;
	if (my_right == INITIATOR) {
		kb_image ^= IP_OBJ;
		image = kb_image;
	} else {
		di_image ^= IP_OBJ;	/* Toggle the Interrupt Process bit */
		image = di_image;
	}
	bzero((char *) &ud, sizeof ud);
	ud.echo_sw = cur_emode;
	ud.type_sw = CTRL_OBJ;
	ud.updates.co_list.co_name = my_signal_obj;
	ud.updates.co_list.co_type = 1;	/* Boolean Update */
	ud.updates.co_list.co_cmd.bool_update.value = &image;
	ud.updates.co_list.co_cmd.bool_update.val_count = KB_SIZE;
	ud.updates.co_list.co_cmd.bool_update.mask = &int_mask;
	ud.updates.co_list.co_cmd.bool_update.mask_count = KB_SIZE;
	send_queue(ud);
}

vt_set_nego(image, mask)	/* Update NA/NI control object as in image */
	char image;
	char mask;
{

	TEXT_UPDATE ud;
	char e_image;

	bzero((char *) &ud, sizeof ud);
	ud.echo_sw = cur_emode;
	ud.type_sw = CTRL_OBJ;
	ud.updates.co_list.co_name = my_echo_obj;
	ud.updates.co_list.co_type = 1;	/* Boolean */
	e_image = image;
	ud.updates.co_list.co_cmd.bool_update.value = &e_image;
	ud.updates.co_list.co_cmd.bool_update.val_count = NA_SIZE;
	ud.updates.co_list.co_cmd.bool_update.mask = &mask;
	ud.updates.co_list.co_cmd.bool_update.mask_count = NA_SIZE;
	send_queue(ud);
	vtsend();		/* Since we're bypassing normal keyboard entry */
}

vt_echo(echo)
	int echo;
{

	if (!telnet_profile) {
		advise(LLOG_NOTICE, NULLCP, "not using TELNET profile");
		return;
	}
	if ((ni_image & ECHO_OBJ) != (nego_state & ECHO_OBJ)) {
		advise(LLOG_NOTICE, NULLCP, "negotiation in progress, try again later...");
		return;
	}

	if (echo != ((nego_state & ECHO_OBJ) ? 1 : 0)) {
		if (echo)
			ni_image |= ECHO_OBJ;
		else
			ni_image &= ~ECHO_OBJ;

		vt_set_nego(ni_image, ECHO_OBJ);	/* Set proper UNIX echo state when reponse
							   is received. */
	} else
		advise(LLOG_NOTICE, NULLCP, "already using %s echoing", echo ? "remote" : "local");
}

vt_rem_echo(img_addr)		/* Request Remote Echo Mode.  Parameter is pointer to image byte. */
	char *img_addr;
{
	*img_addr |= ECHO_OBJ;
	vt_set_nego(*img_addr, ECHO_OBJ);
}

vt_sup_ga(img_addr)		/* Request Suppress Go Ahead */
	char *img_addr;
{
	*img_addr |= SUP_GA;
	vt_set_nego(*img_addr, SUP_GA);
}

/* ARGSUSED */
vt_break(vec)
	char **vec;
{
#ifdef VT_BREAK
	if (!do_break) {
		advise(LLOG_NOTICE, NULLCP, "VT-BREAK Functional Unit Not Chosen");
		return OK;
	}
	(void) tmode(2);
	vt_clr_obj();		/* Initialize all control objects */
	vbrkreq();
#else
	TEXT_UPDATE ud;

	mask = BRK_OBJ;
	kb_image ^= BRK_OBJ;	/* Can Only be called by User side */
	image = kb_image;

	bzero((char *) ud, sizeof *ud);
	ud.echo_sw = cur_emode;
	ud.type_sw = CTRL_OBJ;
	ud.updates.co_list.co_name = my_signal_obj;
	ud.updates.co_list.co_type = 1;	/* Boolean Update */
	ud.updates.co_list.co_cmd.bool_update.value = &image;
	ud.updates.co_list.co_cmd.bool_update.val_count = KB_SIZE;
	ud.updates.co_list.co_cmd.bool_update.mask = &mask;
	ud.updates.co_list.co_cmd.bool_update.mask_count = KB_SIZE;
	send_queue(ud);
	vtsend();
#endif

	return OK;
}

/* ARGSUSED */
vt_ayt(vec)			/* Send Are You There */
	char **vec;
{

	TEXT_UPDATE ud;
	char mask;
	char image;

	if (!telnet_profile) {
		advise(LLOG_NOTICE, NULLCP, "not using TELNET profile");
		return NOTOK;
	}
	mask = AYT_OBJ;
	kb_image ^= AYT_OBJ;	/* Can only be called by User side */
	image = kb_image;
	bzero((char *) &ud, sizeof ud);
	ud.echo_sw = cur_emode;
	ud.type_sw = CTRL_OBJ;
	ud.updates.co_list.co_name = my_signal_obj;
	ud.updates.co_list.co_type = 1;	/* Boolean Update */
	ud.updates.co_list.co_cmd.bool_update.value = &image;
	ud.updates.co_list.co_cmd.bool_update.val_count = KB_SIZE;
	ud.updates.co_list.co_cmd.bool_update.mask = &mask;
	ud.updates.co_list.co_cmd.bool_update.mask_count = KB_SIZE;
	send_queue(ud);
	vtsend();

	return OK;
}

switch_rep(rep_num)		/* Change to specified repertoire. Switching is done by sending a
				   Write Attribute NDQ. */
	int rep_num;
{

	TEXT_UPDATE ud;

	if (rep_num == 1)
		transparent = 0;
	else
		transparent = 1;

	bzero((char *) &ud, sizeof ud);
	ud.echo_sw = cur_emode;
	ud.type_sw = DISPLAY_OBJ;
	ud.updates.do_list.do_name = my_displayobj;
	ud.updates.do_list.do_type = DO_ATTR;
	ud.updates.do_list.do_cmd.wrt_attrib.attr_id = 0;
	ud.updates.do_list.do_cmd.wrt_attrib.attr_val = rep_num;	/* Rep Number */
	ud.updates.do_list.do_cmd.wrt_attrib.attr_ext = 2;	/* Modal Extent */
	send_queue(ud);
	vtsend();
}

vt_repertoire(repertoire)
	int repertoire;
{
	if (!telnet_profile) {
		advise(LLOG_NOTICE, NULLCP, "not using TELNET profile");
		return;
	}

	if (repertoire != transparent) {
		if (repertoire)
			ni_image |= (DISP_BIN | KBD_BIN);
		else
			ni_image &= ~(DISP_BIN | KBD_BIN);
		vt_set_nego(ni_image, DISP_BIN | KBD_BIN);
	} else
		advise(LLOG_NOTICE, NULLCP, "already using %s repertoire",
		       transparent ? "BINARY" : "ASCII");
}

vt_clr_obj()
{				/* Set TELNET Profile Control Objects to 0 */
	kb_image = di_image = 0;
	nego_state = ni_image = na_image = 0;
	sync_image = ga_image = 0;
}

 /*ARGSUSED*/
vt_sync(vec)				/* Send TELNET SYNC signal (test for UDQ & typed data) */
	char **vec;
{

	PE udqp;
	TEXT_UPDATE ud;
	char mask, image;

	mask = SYNC;
	sync_image ^= SYNC;
	image = sync_image;
	bzero((char *) &ud, sizeof ud);
	ud.echo_sw = cur_emode;
	ud.type_sw = CTRL_OBJ;
	ud.updates.co_list.co_name = "SY";
	ud.updates.co_list.co_type = 1;
	ud.updates.co_list.co_cmd.bool_update.value = &image;
	ud.updates.co_list.co_cmd.bool_update.val_count = SYNC_SIZE;
	ud.updates.co_list.co_cmd.bool_update.mask = &mask;
	ud.updates.co_list.co_cmd.bool_update.mask_count = SYNC_SIZE;
	if (build_UDQPDU_UDQpdu(&udqp, 1, NULL, NULLCP, (PEPYPARM) & ud) == NOTOK)
		adios(NULLCP, "UDQ build failure");
	udqp->pe_context = 1;
	(void) do_event(VDATreq_u, udqp);
	pe_free(udqp);
	return OK;
}