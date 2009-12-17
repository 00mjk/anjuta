#include <gtk/gtk.h>
#include <string.h>
#include "egg-datetime.h"
#include "main.h"

guint64 julian=0, start_jul=0, stop_jul=0;
void add_edit_completed_toggled(GtkWidget *checkbox, GtkWidget *rlabel);

void add_edit_option_changed(GtkComboBox *option, gpointer data);

static
void check_length(GtkWidget *entry, GtkWidget *button)
{
	if(strlen(gtk_entry_get_text(GTK_ENTRY(entry))) > 0)
	{
		gtk_widget_set_sensitive(button, TRUE);    
	}
	else
	{
		gtk_widget_set_sensitive(button, FALSE);    
	}
}

#if 0
static
void add_edit_comment_keybord_focus_in_event(GtkWidget *menu)
{
	/* No default selection for now */
	
	GtkTextIter biter, eiter;
	GtkTextBuffer *tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(menu));
	gtk_text_buffer_get_end_iter(tb, &eiter);
	gtk_text_buffer_get_start_iter(tb, &biter);
	/* set the cursor posistion */
	gtk_text_buffer_place_cursor(tb,&biter);
#if 0
	gtk_text_buffer_place_cursor(tb,&eiter);
	if(GTK_MINOR_VERSION >= 3) gtk_text_buffer_select_range(tb, &eiter,&biter); 
#endif
}
#endif

/* this function is called when the egg_datetime entry changes	*
 * It checks if there is an end date and an end time and makes 	*
 * the notify button sensitive if so.				*/

static
void date_time_changed(GtkWidget *eg, GtkWidget *notify)
{
	int minute, hour;
	egg_datetime_get_time(EGG_DATETIME(eg), &hour, &minute, NULL);
	if(egg_get_nodate(EGG_DATETIME(eg)))
	{
		gtk_widget_set_sensitive(notify, FALSE);
	}
	else gtk_widget_set_sensitive(notify, TRUE);    
}

static gboolean
RowSeparatorFunc (GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	int n, col = gtk_tree_model_iter_n_children (model, NULL);
	gchar *path = gtk_tree_model_get_string_from_iter (model, iter);
	sscanf (path, "%d", &n);
	g_free (path);
	return col - 2 == n;
}

void gui_add_todo_item(GtkWidget *useless, gpointer data, guint32 openid){
	GtkWidget *dialog;
	GtkWidget *done_check=NULL;
	GtkWidget *summary;
	GtkWidget *priority;
	GtkWidget *label;
	GtkWidget *text_view;
	GtkWidget *option;
	GtkWidget *vbox, *hbox2, *vbox2;
	GtkTreeIter iter;
	GtkTextBuffer *buffer;
	GtkTextIter first, last;
	GtkWidget *addbut;
	GtkWidget *vp;    
	GtkWidget *cals[2];
	GtkSizeGroup *sglabel, *sgdd;
	GtkWidget *rlabel=NULL, *notify_cb, *llabel=NULL;
	gchar *tempstr;
	guint64 idvalue;
	int i, k;
	int edit = GPOINTER_TO_INT(data);
	GtkTreeSelection *selection;
	GtkTreeModel *model = mw.sortmodel;
	GTodoItem *item = NULL;
	gchar *category = NULL;
	start_jul = stop_jul = 0;
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(mw.treeview));	
	if(edit > 0){
		if(!gtk_tree_selection_get_selected(selection, &model, &iter))
		{
			message_box( _("You need to select a to do item before you can edit it"),"",GTK_MESSAGE_INFO);
			return;
		}
		gtk_tree_model_get(model, &iter, ID, &idvalue, -1);
		item = gtodo_client_get_todo_item_from_id(cl, idvalue);
		if(item == NULL) return;
	}
	else if (edit == -1)
	{
		item = gtodo_client_get_todo_item_from_id(cl, openid);
		if(item == NULL) return;
		edit = 1;
	}
	/* Size Groups for the labels and the priority date and category selector */
	sglabel = gtk_size_group_new(GTK_SIZE_GROUP_BOTH);
	sgdd = gtk_size_group_new(GTK_SIZE_GROUP_BOTH);


	if(!edit){
		dialog = gtk_dialog_new_with_buttons(_("Add Item"), GTK_WINDOW(mw.window), 
				GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, NULL);
	}
	else  	dialog = gtk_dialog_new_with_buttons(_("Edit Item"), GTK_WINDOW(mw.window), 
			GTK_DIALOG_DESTROY_WITH_PARENT, NULL);
	gtk_window_set_default_size (GTK_WINDOW (dialog), 520, 600);
	
	if(gtodo_client_get_read_only(cl))
	{
		addbut = gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_CANCEL);
	}
	else 
	{
		gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
		if(!edit){
			addbut = gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_ADD, GTK_RESPONSE_ACCEPT);
		}
		else{
			addbut = gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_APPLY, GTK_RESPONSE_ACCEPT);
		}

		gtk_widget_set_sensitive(addbut, FALSE);
	}

	/* the main hbox, that spilts the window in 2 */

	/* the vbox for the selectors, buttons */
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), vbox, TRUE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 9);

	/* summary label */
	hbox2 = gtk_hbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, TRUE, 6);
	label = gtk_label_new(_("Summary:"));
	tempstr = g_strdup_printf("<b>%s</b>", _("Summary:"));
	gtk_label_set_markup(GTK_LABEL(label), tempstr);
	g_free(tempstr);
	gtk_misc_set_alignment(GTK_MISC(label), 1,0.5);
	gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, TRUE,0);
	gtk_size_group_add_widget(sglabel, label);
	/* summary entry box */
	summary = gtk_entry_new();
	gtk_entry_set_max_length (GTK_ENTRY(summary), 64);

	gtk_box_pack_start(GTK_BOX(hbox2), summary, TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT(summary), "changed", G_CALLBACK(check_length), addbut);

	/* add category switch */
	/* hbox for the label switch and toggle but */
	hbox2 = gtk_hbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, TRUE, 12); 

	/* option menu label */
	label = gtk_label_new(_("Category:"));
	gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, TRUE, 0);
	gtk_misc_set_alignment(GTK_MISC(label),1, 0.5);
	gtk_size_group_add_widget(sglabel, label);

	/* option menu */
	option = gtk_combo_box_new_text();

	for(i = 0, k = 0; i < (categorys); i++)
	{
		gtk_combo_box_append_text (GTK_COMBO_BOX (option), mw.mitems[i]->date);
		if(edit){
			if(!g_utf8_collate(gtodo_todo_item_get_category(item), mw.mitems[i]->date))
			{
				k = i;
			}
		}
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(option), k);

	gtk_combo_box_append_text (GTK_COMBO_BOX (option), "");
	gtk_combo_box_append_text (GTK_COMBO_BOX (option), _("Edit Categories"));
	gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (option), RowSeparatorFunc, NULL, NULL);

	g_free(category);

	g_signal_connect(G_OBJECT(option), "changed", G_CALLBACK( add_edit_option_changed ), NULL);
	if(!edit)gtk_combo_box_set_active(GTK_COMBO_BOX(option), gtk_combo_box_get_active(GTK_COMBO_BOX (mw.option))-2);
	gtk_box_pack_start(GTK_BOX(hbox2), option, TRUE, TRUE, 0);
	gtk_size_group_add_widget(sgdd, option);

	/* add the calender label */
	hbox2 = gtk_hbox_new(FALSE, 12);
	label = gtk_label_new(_("Due date:"));
	gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, TRUE, 6); 
	gtk_misc_set_alignment(GTK_MISC(label), 1,0.5);
	gtk_size_group_add_widget(sglabel, label);

	cals[0] = egg_datetime_new();
	gtk_box_pack_start(GTK_BOX(hbox2), cals[0],TRUE, TRUE, 0);    
	egg_datetime_set_time(EGG_DATETIME(cals[0]),-1,0,0);
	egg_set_nodate(EGG_DATETIME(cals[0]), TRUE);

	hbox2 = gtk_hbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, TRUE, 6);    
	/* label */
	/*	//    label = gtk_label_new(_("Notify when due"));*/
	label = gtk_label_new(NULL);
	gtk_size_group_add_widget(sglabel, label);
	gtk_misc_set_alignment(GTK_MISC(label), 1,0.5);
	gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, TRUE, 0);    
	notify_cb = gtk_check_button_new_with_label(_("Notify when due"));
	if(edit) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(notify_cb), gtodo_todo_item_get_notify(item));
	else gtk_widget_set_sensitive(GTK_WIDGET(notify_cb), FALSE);
	gtk_box_pack_start(GTK_BOX(hbox2), notify_cb, FALSE, TRUE, 0);    

	g_signal_connect(G_OBJECT(cals[0]),   "date-changed", G_CALLBACK(date_time_changed), notify_cb);
	g_signal_connect(G_OBJECT(cals[0]),   "time-changed", G_CALLBACK(date_time_changed), notify_cb);
	/* Add the priority bar */
	/* hbox to hold it*/
	hbox2 = gtk_hbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, TRUE, 6);    

	/* label */
	label = gtk_label_new(_("Priority:"));
	gtk_size_group_add_widget(sglabel, label);
	gtk_misc_set_alignment(GTK_MISC(label), 1,0.5);
	gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, TRUE, 0);    

	/* fixme move pmenu and menui up */
	{
		priority = gtk_combo_box_new_text();

		gtk_combo_box_append_text (GTK_COMBO_BOX (priority), "High");
		gtk_combo_box_append_text (GTK_COMBO_BOX (priority), "Medium");
		gtk_combo_box_append_text (GTK_COMBO_BOX (priority), "Low");

		gtk_combo_box_set_active (GTK_COMBO_BOX (priority), 1);
		gtk_box_pack_start(GTK_BOX(hbox2), priority,TRUE, TRUE, 0);
		gtk_size_group_add_widget(sgdd, priority);
	}

	/* comment label */
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), vbox2, TRUE, TRUE, 6);
	label = gtk_label_new(_("Comment:"));
	tempstr = g_strdup_printf("<b>%s</b>", _("Comment:"));
	gtk_label_set_markup(GTK_LABEL(label), tempstr);
	g_free(tempstr);
	gtk_misc_set_alignment(GTK_MISC(label), 0,0.5);
	gtk_box_pack_start(GTK_BOX(vbox2), label, FALSE, TRUE, 6);
	gtk_size_group_add_widget(sglabel, label);
	/* comment text field */
	/* make sure it has nice scrollbars and a nice box around it (giving the 3d effect of the borders) */
	text_view = gtk_text_view_new();
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD);
	label = gtk_scrolled_window_new(NULL, NULL);    
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(label), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(label), text_view);
	vp = gtk_viewport_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(vp), label);
	gtk_box_pack_start(GTK_BOX(vbox2), vp, TRUE, TRUE, 0);
#if 0
	g_signal_connect(G_OBJECT(text_view), "focus-in-event", G_CALLBACK(add_edit_comment_keybord_focus_in_event), NULL);
#endif

	/* the completed check button */
	if(edit)
	{
		hbox2 = gtk_hbox_new(FALSE, 12);
		done_check = gtk_check_button_new_with_label(_("Completed"));
		gtk_box_pack_end(GTK_BOX(hbox2), done_check, FALSE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, TRUE, 6);
		llabel = gtk_label_new("");

		hbox2 = gtk_hbox_new(FALSE,12);
		tempstr = g_strdup_printf("<i>%s %s</i>", _("started:"), _("N/A"));
		gtk_label_set_markup(GTK_LABEL(llabel), tempstr);
		gtk_box_pack_start(GTK_BOX(hbox2),llabel, TRUE, TRUE, 6);
		gtk_misc_set_alignment(GTK_MISC(llabel), 0,0.5);	
		g_free(tempstr);

		rlabel = gtk_label_new("");	
		tempstr = g_strdup_printf("<i>%s %s</i>", _("stopped:"), _("N/A"));
		gtk_label_set_markup(GTK_LABEL(rlabel), tempstr);
		gtk_box_pack_start(GTK_BOX(hbox2),rlabel, TRUE, TRUE, 6);
		gtk_misc_set_alignment(GTK_MISC(rlabel), 1,0.5);
		g_free(tempstr);
		gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, TRUE, 6);
	}

	gtk_widget_grab_default(addbut);
	gtk_entry_set_activates_default(GTK_ENTRY(summary), TRUE);

	gtk_widget_show_all(GTK_WIDGET(GTK_DIALOG(dialog)));

	/* this needs to be after show all or the show_all will make this command useless 
	//    egg_datetime_set_display_mode(EGG_DATETIME(cals[0]), EGG_DATETIME_DISPLAY_DATE);
	*/

	/* load value's */
	if(edit){
		if(gtodo_todo_item_get_due_date(item) != NULL)
		{
			egg_set_nodate(EGG_DATETIME(cals[0]), FALSE);
			egg_datetime_set_from_gdate(EGG_DATETIME(cals[0]), (GDate *)gtodo_todo_item_get_due_date(item));
			egg_datetime_set_time(EGG_DATETIME(cals[0]), gtodo_todo_item_get_due_time_houre(item), gtodo_todo_item_get_due_time_minute(item),0);
		}
		else {
			egg_set_nodate(EGG_DATETIME(cals[0]), TRUE);
		}

		/* make sure  that the calendar is insensitive when needed */
		gtk_entry_set_text(GTK_ENTRY(summary), gtodo_todo_item_get_summary(item));
		gtk_combo_box_set_active(GTK_COMBO_BOX(priority), 2-gtodo_todo_item_get_priority(item));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(done_check), gtodo_todo_item_get_done(item));
		gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view)), gtodo_todo_item_get_comment(item), -1);
		{
			gchar *buffer, *buffer1;
			if((buffer1 = gtodo_todo_item_get_stop_date_as_string(item)) == NULL)  buffer1 = g_strdup_printf(_("N/A"));
			if((buffer = gtodo_todo_item_get_start_date_as_string(item)) == NULL)  buffer = g_strdup_printf(_("N/A"));		

			tempstr = g_strdup_printf("<i>%s %s</i>", _("started:"), buffer);
			gtk_label_set_markup(GTK_LABEL(llabel),tempstr);
			g_free(tempstr);
			
			tempstr = g_strdup_printf("<i>%s %s</i>", _("stopped:"), buffer1);
			gtk_label_set_markup(GTK_LABEL(rlabel),tempstr);
			g_free(tempstr);
			g_free(buffer);
			g_free(buffer1);
		}
	}

	if(gtodo_client_get_read_only(cl))
	{
	gtk_widget_set_sensitive(vbox, FALSE);
	}                                                           
	else
	{
	gtk_editable_select_region(GTK_EDITABLE(summary), 0,-1);
	}
	switch(gtk_dialog_run(GTK_DIALOG(dialog))){
		case GTK_RESPONSE_ACCEPT:
			break;
		default:
			gtk_widget_destroy(dialog);
			return;
	}

	/* a free last id.. this is always needed.. because edit works with delete */
	if(!edit) item = gtodo_client_create_new_todo_item(cl);
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
	gtk_text_buffer_get_iter_at_offset (buffer, &first, 0);
	gtk_text_buffer_get_iter_at_offset (buffer, &last, -1);

	gtodo_todo_item_set_category(item, mw.mitems[gtk_combo_box_get_active(GTK_COMBO_BOX(option))]->date);
	gtodo_todo_item_set_priority(item, 2-gtk_combo_box_get_active(GTK_COMBO_BOX(priority)));
	if(edit) gtodo_todo_item_set_done(item, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(done_check)));
	/*( COMPLETED_DATE */
	/*    if(edit)
	      {
	      if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(done_check)))
	      {
	      gtodo_todo_item_set_stop_date_today(item);
	      } 
	      }
	      */    if(!egg_get_nodate(EGG_DATETIME(cals[0])))
	{
		GDate *comp;
		comp = g_date_new();
		egg_datetime_get_as_gdate(EGG_DATETIME(cals[0]), comp);
		if(g_date_valid(comp))
		{
			julian = g_date_get_julian(comp);
			gtodo_todo_item_set_due_date_as_julian(item, julian);
		}
		g_date_free(comp);
		{
			gint houre, minute;
			egg_datetime_get_time(EGG_DATETIME(cals[0]), &houre, &minute, NULL);
			gtodo_todo_item_set_due_time_houre(item, (int)houre);
			gtodo_todo_item_set_due_time_minute(item, (int)minute);	    
		}
	}
	      else gtodo_todo_item_set_due_date_as_julian(item, GTODO_NO_DUE_DATE);

	      gtodo_todo_item_set_summary(item,(gchar *)gtk_entry_get_text(GTK_ENTRY(summary)));
	      gtodo_todo_item_set_notify(item, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(notify_cb)));    
	      tempstr = gtk_text_buffer_get_text(buffer, &first, &last, FALSE);
	      gtodo_todo_item_set_comment(item,tempstr);
	      g_free(tempstr);



	      /* testing blocking the file changed */
	      /* it seems to work.. so I keep it here */
	      gtodo_client_block_changed_callback(cl);
	      if(edit) gtodo_client_edit_todo_item(cl,item);
	      else gtodo_client_save_todo_item(cl, item);       
	      gtk_list_store_clear(mw.list);
	      load_category();
	      gtodo_client_unblock_changed_callback(cl);
	      /* end test */
	      gtk_widget_destroy(dialog);
}

void add_edit_completed_toggled(GtkWidget *checkbox, GtkWidget *rlabel)
{	
	GDate *date, *date1;
	gchar buffer[64], buffer1[64];
	gchar *tempstr;
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbox))){
		date = g_date_new();
		g_date_set_time_t(date, time(NULL));
		stop_jul= g_date_get_julian(date);
		g_date_free(date);
	}
	else stop_jul = 0;

	if(start_jul){
		date = g_date_new_julian(start_jul);
		g_date_strftime(buffer, 64, "%x", date);
		g_date_free(date);
	}
	else strcpy(buffer, "N/A");
	if(stop_jul)
	{
		date1= g_date_new_julian(stop_jul);
		g_date_strftime(buffer1, 64, "%x", date1);	
		g_date_free(date1);
	}
	else strcpy(buffer1, "N/A");
	
	tempstr = g_strdup_printf("<i>%s %s \t%s %s</i>",
							  _("started:"), _("stopped:"),
							  buffer, buffer1);
	gtk_label_set_markup(GTK_LABEL(rlabel), tempstr);
	g_free(tempstr);
}

void add_edit_option_changed(GtkComboBox *option, gpointer data)
{
	int i = gtk_combo_box_get_active(option);    
	if(i == categorys +1)
	{
		category_manager();

		while (gtk_tree_model_iter_n_children (gtk_combo_box_get_model (option), NULL) > 0) {
			gtk_combo_box_remove_text (option, 0);
		}

		for(i = 0; i < (categorys); i++)
		{
			gtk_combo_box_append_text (option, mw.mitems[i]->date);
		}
		gtk_combo_box_set_active(GTK_COMBO_BOX(option), 0);	

		gtk_combo_box_append_text (option, "");
		gtk_combo_box_append_text (option, _("Edit Categories"));
		gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (option), RowSeparatorFunc, NULL, NULL);

		gtk_widget_show_all(GTK_WIDGET(option));
	}
}
