/*!  \file     gui_operations_menu.c
*    \brief    GUI for the operations menu
*    Created:  19/11/2018
*    Author:   Mathieu Stephan
*/
#include "gui_operations_menu.h"
#include "gui_dispatcher.h"
#include "gui_carousel.h"
#include "defines.h"
// Menu layout
const uint16_t operations_menu_pic_ids[] = {GUI_CLONE_ICON_ID, GUI_CHANGE_PIN_ICON_ID, GUI_ERASE_USER_ICON_ID, GUI_BACK_ICON_ID};
const uint16_t operations_menu_text_ids[] = {GUI_CLONE_TEXT_ID, GUI_CHANGE_PIN_TEXT_ID, GUI_ERASE_USER_TEXT_ID, GUI_BACK_TEXT_ID};
// Currently selected item
uint16_t gui_operations_menu_selected_item = 0;


/*! \fn     gui_operations_menu_reset_state(void)
*   \brief  Reset menu state
*/
void gui_operations_menu_reset_state(void)
{
    gui_operations_menu_selected_item = 2;
}

/*! \fn     gui_operations_menu_event_render(wheel_action_ret_te wheel_action)
*   \brief  Render GUI depending on event received
*   \param  wheel_action    Wheel action received
*   \return TRUE if screen rendering is required
*/
BOOL gui_operations_menu_event_render(wheel_action_ret_te wheel_action)
{
    /* How many elements our menu has */
    const uint16_t nb_menu_elements = sizeof(operations_menu_pic_ids)/sizeof(operations_menu_pic_ids[0]);
    
    if (wheel_action == WHEEL_ACTION_NONE)
    {
        gui_carousel_render(nb_menu_elements, operations_menu_pic_ids, operations_menu_text_ids, gui_operations_menu_selected_item, 0);
    }
    else if (wheel_action == WHEEL_ACTION_UP)
    {
        gui_carousel_render_animation(nb_menu_elements, operations_menu_pic_ids, operations_menu_text_ids, gui_operations_menu_selected_item, TRUE);
        if (gui_operations_menu_selected_item-- == 0)
        {
            gui_operations_menu_selected_item = nb_menu_elements-1;
        }                
        gui_carousel_render(nb_menu_elements, operations_menu_pic_ids, operations_menu_text_ids, gui_operations_menu_selected_item, 0);
    }
    else if (wheel_action == WHEEL_ACTION_DOWN)
    {        
            gui_carousel_render_animation(nb_menu_elements, operations_menu_pic_ids, operations_menu_text_ids, gui_operations_menu_selected_item, FALSE);
            if (++gui_operations_menu_selected_item == nb_menu_elements)
            {
                gui_operations_menu_selected_item = 0;
            }
            gui_carousel_render(nb_menu_elements, operations_menu_pic_ids, operations_menu_text_ids, gui_operations_menu_selected_item, 0);
    }
    else if (wheel_action == WHEEL_ACTION_SHORT_CLICK)
    {
        /* Get selected icon */
        uint16_t selected_icon = operations_menu_pic_ids[gui_operations_menu_selected_item];
        
        /* Switch on the selected icon ID */
        switch (selected_icon)
        {
            case GUI_BACK_ICON_ID:   
            {
                gui_dispatcher_set_current_screen(GUI_SCREEN_MAIN_MENU, FALSE, GUI_OUTOF_MENU_TRANSITION);
                gui_operations_menu_selected_item = 0;
                return TRUE;
            }                
            default: break;
        }
    }
    
    
    return FALSE;
}