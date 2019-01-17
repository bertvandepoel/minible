/*!  \file     gui_carousel.c
*    \brief    GUI carousel rendering functions
*    Created:  17/11/2018
*    Author:   Mathieu Stephan
*/
#include "platform_defines.h"
#include "gui_carousel.h"
#include "driver_timer.h"
#include "defines.h"
#include "sh1122.h"
#include "main.h"
/* Carousel spacing depending on number of elemnts */
const uint16_t gui_carousel_x_anim_steps[] = {0,0,0,CAROUSEL_X_STEP_ANIM(3),CAROUSEL_X_STEP_ANIM(4),CAROUSEL_X_STEP_ANIM(5),CAROUSEL_X_STEP_ANIM(6),CAROUSEL_X_STEP_ANIM(7),CAROUSEL_X_STEP_ANIM(8)};
const uint16_t gui_carousel_inter_icon_spacing[] = {0,0,0,CAROUSEL_IS_SM(3),CAROUSEL_IS_SM(4),CAROUSEL_IS_SM(5),CAROUSEL_IS_SM(6),CAROUSEL_IS_SM(7),CAROUSEL_IS_SM(8)};
const uint16_t gui_carousel_left_spacing[] = {0,0,0,CAROUSEL_LS_SM(3),CAROUSEL_LS_SM(4),CAROUSEL_LS_SM(5),CAROUSEL_LS_SM(6),CAROUSEL_LS_SM(7),CAROUSEL_LS_SM(8)};


/*! \fn     gui_carousel_render(uint16_t nb_elements, const uint16_t* pic_ids, const uint16_t* text_ids, uint16_t selected_id, int16_t anim_step)
*   \brief  Carousel rendering function
*   \param  nb_elements     Number of elements in the carousel
*   \param  pic_ids         Array of the icon IDs
*   \param  text_ids        Array of the text IDs
*   \param  selected_id     Currently selected icon ID
*   \param  anim_step       Animation step
*/
void gui_carousel_render(uint16_t nb_elements, const uint16_t* pic_ids, const uint16_t* text_ids, uint16_t selected_id, int16_t anim_step)
{
    /* Clear frame buffer */
    sh1122_clear_frame_buffer(&plat_oled_descriptor);
    
    /* Allow wrapping */
    plat_oled_descriptor.screen_wrapping_allowed = TRUE;
    
    /* Compute most left icon index based on selected icon */
    int16_t cur_icon_index = selected_id - (nb_elements/2);
    if (cur_icon_index < 0)
    {
        cur_icon_index += nb_elements;
    }
    
    /* Start displaying icons */
    uint16_t cur_display_x = gui_carousel_left_spacing[nb_elements] - anim_step*gui_carousel_x_anim_steps[nb_elements];    
    for (uint16_t i = 0; i < nb_elements; i++)
    {
        if (i == nb_elements/2)
        {
            /* Center icon */
            sh1122_display_bitmap_from_flash(&plat_oled_descriptor, cur_display_x, CAROUSEL_Y_ALIGN-(CAROUSEL_BIG_EDGE-abs(anim_step)*CAROUSEL_Y_ANIM_STEP*2)/2, pic_ids[cur_icon_index] + abs(anim_step), TRUE);
            cur_display_x += CAROUSEL_BIG_EDGE - abs(anim_step)*CAROUSEL_Y_ANIM_STEP*2;
        }
        else if (i == (nb_elements/2)-1)
        {
            /* Left to the center icon */
            if (anim_step < 0)
            {
                sh1122_display_bitmap_from_flash(&plat_oled_descriptor, cur_display_x, CAROUSEL_Y_ALIGN-(CAROUSEL_MID_EDGE-anim_step*CAROUSEL_Y_ANIM_STEP*2)/2, pic_ids[cur_icon_index] + (CAROUSEL_NB_SCALED_ICONS/2) + anim_step, TRUE);
                cur_display_x += CAROUSEL_MID_EDGE - anim_step*CAROUSEL_Y_ANIM_STEP*2;
            }
            else
            {
                sh1122_display_bitmap_from_flash(&plat_oled_descriptor, cur_display_x, CAROUSEL_Y_ALIGN-(CAROUSEL_MID_EDGE-anim_step*CAROUSEL_Y_ANIM_STEP)/2, pic_ids[cur_icon_index] + (CAROUSEL_NB_SCALED_ICONS/2) + anim_step, TRUE);
                cur_display_x += CAROUSEL_MID_EDGE - anim_step*CAROUSEL_Y_ANIM_STEP;
            }
        }
        else if (i == (nb_elements/2)+1)
        {
            /* Right to the center icon */
            if (anim_step < 0)
            {
                sh1122_display_bitmap_from_flash(&plat_oled_descriptor, cur_display_x, CAROUSEL_Y_ALIGN-(CAROUSEL_MID_EDGE+anim_step*CAROUSEL_Y_ANIM_STEP)/2, pic_ids[cur_icon_index] + (CAROUSEL_NB_SCALED_ICONS/2) - anim_step, TRUE);
                cur_display_x += CAROUSEL_MID_EDGE + anim_step*CAROUSEL_Y_ANIM_STEP;
            }
            else
            {
                sh1122_display_bitmap_from_flash(&plat_oled_descriptor, cur_display_x, CAROUSEL_Y_ALIGN-(CAROUSEL_MID_EDGE+anim_step*CAROUSEL_Y_ANIM_STEP*2)/2, pic_ids[cur_icon_index] + (CAROUSEL_NB_SCALED_ICONS/2) - anim_step, TRUE);
                cur_display_x += CAROUSEL_MID_EDGE + anim_step*CAROUSEL_Y_ANIM_STEP*2;
            }
        }
        else if (((i == (nb_elements/2)-2) && (anim_step < 0)) || ((i == (nb_elements/2)+2) && (anim_step > 0)))
        {
            sh1122_display_bitmap_from_flash(&plat_oled_descriptor, cur_display_x, CAROUSEL_Y_ALIGN-(CAROUSEL_SMALL_EDGE+abs(anim_step)*CAROUSEL_Y_ANIM_STEP)/2, pic_ids[cur_icon_index] + CAROUSEL_NB_SCALED_ICONS - abs(anim_step) - 1, TRUE);
            cur_display_x += CAROUSEL_SMALL_EDGE + abs(anim_step)*CAROUSEL_Y_ANIM_STEP;
        }
        else
        {
            sh1122_display_bitmap_from_flash(&plat_oled_descriptor, cur_display_x, CAROUSEL_Y_ALIGN-CAROUSEL_SMALL_EDGE/2, pic_ids[cur_icon_index] + CAROUSEL_NB_SCALED_ICONS - 1, TRUE);
            cur_display_x += CAROUSEL_SMALL_EDGE;
        }
        
        /* Increment icon index */
        cur_icon_index++;
        if (cur_icon_index == nb_elements)
        {
            cur_icon_index = 0;
        }
        
        /* Add spacing */
        cur_display_x += gui_carousel_inter_icon_spacing[nb_elements];
    }
    
    /* Add bottom text: during animation display next icon text */
    cust_char_t* temp_string;
    if (anim_step > 0)
    {
        selected_id++;
        if (selected_id == nb_elements)
        {
            selected_id = 0;
        }
    } 
    else if (anim_step < 0)
    {
        selected_id--;
        if (selected_id == UINT16_MAX)
        {
            selected_id = nb_elements - 1;
        }
    }
    custom_fs_get_string_from_file(text_ids[selected_id], &temp_string);
    sh1122_put_string_xy(&plat_oled_descriptor, 0, 40, OLED_ALIGN_CENTER, temp_string, TRUE);
    
    /* Flush */
    sh1122_flush_frame_buffer(&plat_oled_descriptor);
    
    /* Prevent wrapping */
    plat_oled_descriptor.screen_wrapping_allowed = FALSE;
}


/*! \fn     gui_carousel_render_animation(uint16_t nb_elements, const uint16_t* pic_ids, const uint16_t* text_ids, uint16_t selected_id, BOOL left_anim)
*   \brief  Carousel animation rendering function
*   \param  nb_elements     Number of elements in the carousel
*   \param  pic_ids         Array of the icon IDs
*   \param  text_ids        Array of the text IDs
*   \param  selected_id     Currently selected icon ID
*   \param  left_anim       Set to TRUE to make animation to the left
*/
void gui_carousel_render_animation(uint16_t nb_elements, const uint16_t* pic_ids, const uint16_t* text_ids, uint16_t selected_id, BOOL left_anim)
{
    for (int16_t i = 1; i <= CAROUSEL_NB_SCALED_ICONS/2; i++)
    {
        if (left_anim != FALSE)
        {
            gui_carousel_render(nb_elements, pic_ids, text_ids, selected_id, -i);
        } 
        else
        {
            gui_carousel_render(nb_elements, pic_ids, text_ids, selected_id, i);
        }
        //timer_delay_ms(1000);
    }
}