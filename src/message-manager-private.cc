/*
    message-manager-private.cc
    Copyright (C) 2000, 2001  Kh. Naba Kumar Singh, Johannes Schmid

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "message-manager-private.h"
#include "message-manager-dock.h"
#include "preferences.h"

extern "C"
{
	#include "utilities.h"
};

#include <zvt/zvtterm.h>
#include <pwd.h>

// MessageSubwindow (base class for AnjutaMessageWindow and TerminalWindow:

MessageSubwindow::MessageSubwindow(AnjutaMessageManager* p_amm, int p_type_id, string p_type, string p_pixmap)
{	
	g_return_if_fail(p_amm != NULL);
	
	// Create menuitem
	m_menuitem = gtk_check_menu_item_new_with_label(p_type.c_str());
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(m_menuitem), true);
	connect_menuitem_signal(m_menuitem, this);
	gtk_widget_show(m_menuitem);
	gtk_menu_append(GTK_MENU(p_amm->intern->popupmenu), m_menuitem);
	
	m_parent = p_amm;
	
	m_type_name = p_type;
	m_type_id = p_type_id;
	m_pixmap = p_pixmap;
	m_is_shown = true;

	p_amm->intern->last_page++;
	m_page_num = p_amm->intern->last_page;
}

AnjutaMessageManager* 
MessageSubwindow::get_parent() const
{
	return m_parent;
}
			
bool 
MessageSubwindow::is_shown() const
{
	return m_is_shown;
}
	
gint 
MessageSubwindow::get_page_num() const
{
	return m_page_num;
}

const string&
MessageSubwindow::get_type() const
{
	return m_type_name;
}

int MessageSubwindow::get_type_id() const
{
	return m_type_id;
}

void MessageSubwindow::activate()
{
	if (!m_is_shown)
	{
		disconnect_menuitem_signal(m_menuitem, this);
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(m_menuitem), true);	
		connect_menuitem_signal(m_menuitem, this);
		show();
		// reorder windows
		vector<bool> win_is_shown;
	
		for (uint i = 0; i < m_parent->intern->msg_windows.size(); i++)
		{
			win_is_shown.push_back(m_parent->intern->msg_windows[i]->is_shown());
			m_parent->intern->msg_windows[i]->hide();
		}
		for (uint i = 0; i < m_parent->intern->msg_windows.size(); i++)
		{
			if (win_is_shown[i])
				m_parent->intern->msg_windows[i]->show();
		}
	}	
	gtk_notebook_set_page(GTK_NOTEBOOK(m_parent->intern->notebook), m_page_num);
	m_parent->intern->cur_msg_win = this;
}

GtkWidget* MessageSubwindow::create_label() const
{
	GtkWidget* label = create_xpm_label_box(GTK_WIDGET(m_parent), (gchar*) m_pixmap.c_str(), FALSE, (gchar*) m_type_name.c_str());
	gtk_widget_ref(label);
	gtk_widget_show(label);
	return label;
}

// MessageWindow:

AnjutaMessageWindow::AnjutaMessageWindow(AnjutaMessageManager* p_amm, int p_type_id, string p_type, string p_pixmap)
	: MessageSubwindow(p_amm, p_type_id, p_type, p_pixmap)
{
	g_return_if_fail(p_amm != NULL);

	m_msg_list = gtk_clist_new(1);
	gtk_widget_show(m_msg_list);
	gtk_clist_columns_autosize (GTK_CLIST(m_msg_list));
	
	m_scrolled_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(m_scrolled_win);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(m_scrolled_win),
						      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	
	gtk_container_add(GTK_CONTAINER(m_scrolled_win), m_msg_list);
	GtkWidget* label = create_label();
	
	gtk_notebook_append_page(GTK_NOTEBOOK(p_amm->intern->notebook), m_scrolled_win, label);
	
	m_cur_line = 0;
}

const vector<string>& 
AnjutaMessageWindow::get_messages() const
{
	return m_messages;
}
	
void
AnjutaMessageWindow::add_to_buffer(char c)
{
	m_line_buffer += c;
}

void
AnjutaMessageWindow::append_buffer()
{
	gtk_clist_freeze (GTK_CLIST(m_msg_list));

	GtkAdjustment* adj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW
						     (m_scrolled_win));

	/* If the scrollbar is almost at the end, */
	/* then only we scroll to the end */
	bool update_adj = false;
	if (adj->value > 0.95 * (adj->upper-adj->page_size) || adj->value == 0)
		update_adj = true;
	
	string message = m_line_buffer;
	m_line_buffer = string();
	m_messages.push_back(message);
	
	// Truncate Message:
	int truncat_mesg = preferences_get_int (get_preferences(), TRUNCAT_MESSAGES);
	int mesg_first = preferences_get_int (get_preferences(), TRUNCAT_MESG_FIRST);
	int mesg_last = preferences_get_int (get_preferences(), TRUNCAT_MESG_LAST);
	
	if (truncat_mesg == FALSE
	    || message.length() <= uint(mesg_first + mesg_last))
	{
		char * msg = new char[message.length() + 1];
		strcpy(msg, message.c_str());
		gtk_clist_append(GTK_CLIST(m_msg_list), &msg);
		delete msg;
	}
	else
	{
		string part1(message.begin(), message.begin() + mesg_first);
		string part2(message.end() - mesg_last, message.end());
		string m1 = part1 + " ................... " + part2;
		char* msg = new char[m1.length() + 1];
		strcpy(msg, m1.c_str());
		gtk_clist_append(GTK_CLIST(m_msg_list), &msg);
		delete msg;		
	}
	
	// Highlite messages:
	int dummy_int;
	char* dummy_fn;
	if (parse_error_line((char*) message.c_str(), &dummy_fn, &dummy_int))
	{
		if (message.find(" warning: ") != message.npos)
		{
			gtk_clist_set_foreground(GTK_CLIST(m_msg_list), m_messages.size() - 1, &m_parent->intern->color_warning);
		}
		else
		{
			gtk_clist_set_foreground(GTK_CLIST(m_msg_list), m_messages.size() - 1, &m_parent->intern->color_error);
		}
	}
	else
	{
		if (message.find(':') != message.npos)
		{
			gtk_clist_set_foreground(GTK_CLIST(m_msg_list), m_messages.size() - 1, &m_parent->intern->color_message1);
		}
		else
		{
			gtk_clist_set_foreground(GTK_CLIST(m_msg_list), m_messages.size() - 1, &m_parent->intern->color_message2);
		}
	}
	delete dummy_fn;
	
	gtk_clist_thaw(GTK_CLIST(m_msg_list));
	if (update_adj) 
		gtk_adjustment_set_value (adj, adj->upper - adj->page_size);
}

void
AnjutaMessageWindow::set_cur_line(int line)
{
	m_cur_line = line;
}

unsigned int
AnjutaMessageWindow::get_cur_line() const
{
	return m_cur_line;
}
			
void
AnjutaMessageWindow::clear()
{
	m_messages.clear();
	gtk_clist_clear(GTK_CLIST(m_msg_list));
}

void 
AnjutaMessageWindow::show()
{
	if (!m_is_shown)
	{
		GtkWidget* label = create_label();
		
		gtk_notebook_append_page(GTK_NOTEBOOK(m_parent->intern->notebook), m_scrolled_win, label);
		gtk_widget_unref(m_scrolled_win);
		
		GList* children = gtk_container_children(GTK_CONTAINER(m_parent->intern->notebook));
		for (uint i = 0; i < g_list_length(children); i++)
		{
			if (g_list_nth(children, i)->data == m_scrolled_win)
			{
				m_page_num = i;
				break;
			}
		}
		
		m_is_shown = true;
	}
}

void 
AnjutaMessageWindow::hide()
{
	if (m_is_shown)
	{
		gtk_widget_ref(m_scrolled_win);
		gtk_container_remove(GTK_CONTAINER(m_parent->intern->notebook), m_scrolled_win);
		
		m_is_shown = false;
	}
}

GtkWidget* AnjutaMessageWindow::get_msg_list()
{
	return m_msg_list;
}
	
// Terminal

#undef ZVT_FONT
#define ZVT_FONT "-adobe-courier-medium-r-normal-*-*-120-*-*-m-*-iso8859-1"
#undef ZVT_SCROLLSIZE
#define ZVT_SCROLLSIZE 200

TerminalWindow::TerminalWindow(AnjutaMessageManager* p_amm, int p_type_id, string p_type, string p_pixmap)
	: MessageSubwindow(p_amm, p_type_id, p_type, p_pixmap)
{	
	g_return_if_fail(p_amm != NULL);
	
	/* A quick hack so that we get a beautiful color terminal */
	setenv("TERM", "xterm", 1);
	
	m_terminal = zvt_term_new();
	zvt_term_set_font_name(ZVT_TERM(m_terminal), ZVT_FONT);
	zvt_term_set_blink(ZVT_TERM(m_terminal), TRUE);
	zvt_term_set_bell(ZVT_TERM(m_terminal), TRUE);
	zvt_term_set_scrollback(ZVT_TERM(m_terminal), ZVT_SCROLLSIZE);
	zvt_term_set_scroll_on_keystroke(ZVT_TERM(m_terminal), TRUE);
	zvt_term_set_scroll_on_output(ZVT_TERM(m_terminal), FALSE);
	zvt_term_set_background(ZVT_TERM(m_terminal), NULL, 0, 0);
	zvt_term_set_wordclass(ZVT_TERM(m_terminal), (unsigned char*) "-A-Za-z0-9/_:.,?+%=");
	gtk_widget_show(m_terminal);

	m_scrollbar = gtk_vscrollbar_new(GTK_ADJUSTMENT(
	  ZVT_TERM(m_terminal)->adjustment));
	GTK_WIDGET_UNSET_FLAGS(m_scrollbar, GTK_CAN_FOCUS);
	
	m_frame = gtk_frame_new(NULL);
	gtk_widget_show (m_frame);
	gtk_frame_set_shadow_type(GTK_FRAME(m_frame), GTK_SHADOW_IN);
	gtk_notebook_append_page(GTK_NOTEBOOK(p_amm->intern->notebook), m_frame, create_label());

	m_hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show (m_hbox);
	gtk_container_add (GTK_CONTAINER(m_frame), m_hbox);
	
	gtk_box_pack_start(GTK_BOX(m_hbox), m_terminal, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(m_hbox), m_scrollbar, FALSE, TRUE, 0);
	gtk_widget_show (m_scrollbar);

	zvterm_reinit_child(ZVT_TERM(m_terminal));
	gtk_signal_connect(GTK_OBJECT(m_terminal), "button_press_event"
	  , GTK_SIGNAL_FUNC(TerminalWindow::zvterm_mouse_clicked), m_terminal);
	gtk_signal_connect (GTK_OBJECT(m_terminal), "child_died"
	  , GTK_SIGNAL_FUNC (TerminalWindow::zvterm_reinit_child), NULL);
	gtk_signal_connect (GTK_OBJECT (m_terminal),"destroy"
	  , GTK_SIGNAL_FUNC (TerminalWindow::zvterm_terminate), NULL);
}
	
void TerminalWindow::show()
{
	if (!m_is_shown)
	{		
		GtkWidget* label = create_label();
		
		gtk_notebook_append_page(GTK_NOTEBOOK(m_parent->intern->notebook), m_frame, label);
		gtk_widget_unref(m_frame);
		
		GList* children = gtk_container_children(GTK_CONTAINER(m_parent->intern->notebook));
		for (uint i = 0; i < g_list_length(children); i++)
		{
			if (g_list_nth(children, i)->data == m_frame)
			{
				m_page_num = i;
				break;
			}
		}
		m_is_shown = true;
	}
}
	
void TerminalWindow::hide()
{
	if (m_is_shown)
	{
		gtk_widget_ref(m_frame);
		gtk_container_remove(GTK_CONTAINER(m_parent->intern->notebook), m_frame);
		m_is_shown = false;
	}
}

gboolean TerminalWindow::zvterm_mouse_clicked(GtkWidget* widget, GdkEvent* event, gpointer user_data)
{
	GtkWidget* terminal = GTK_WIDGET(user_data);
	gtk_widget_grab_focus(terminal);
	return TRUE;
}

extern char **environ;

void TerminalWindow::zvterm_reinit_child(ZvtTerm* term)
{
	struct passwd *pw;
	static GString *shell = NULL;
	static GString *name = NULL;

	if (!shell)
		shell = g_string_new(NULL);
	if (!name)
		name = g_string_new(NULL);
	zvt_term_reset(term, TRUE);
	switch (zvt_term_forkpty(term, ZVT_TERM_DO_UTMP_LOG |  ZVT_TERM_DO_WTMP_LOG))
	{
		case -1:
			break;
		case 0:
			pw = getpwuid(getuid());
			if (pw)
			{
				g_string_assign(shell, pw->pw_shell);
				g_string_assign(name, "-");
			}
			else
			{
				g_string_assign(shell, "/bin/sh");
				g_string_assign(name, "-sh");
			}
			execle (shell->str, name->str, NULL, environ);
		default:
			g_message("Terminal restarted");
			break;
	}
}

void TerminalWindow::zvterm_terminate(ZvtTerm* term)
{
	g_message("Terminating terminal..");
	gtk_signal_disconnect_by_func(GTK_OBJECT(term), GTK_SIGNAL_FUNC(TerminalWindow::zvterm_reinit_child), NULL);
	zvt_term_closepty(term);
}
