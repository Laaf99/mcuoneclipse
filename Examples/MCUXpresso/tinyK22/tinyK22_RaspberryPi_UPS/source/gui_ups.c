/*
 * gui_tempHum.c
 *
 *  Created on: 06.08.2018
 *      Author: Erich Styger
 */

#include "platform.h"
#if PL_CONFIG_USE_UPS && PL_CONFIG_USE_LVGL
#include "gui_ups.h"
#include "LittlevGL/lvgl/lvgl.h"
#include "ups.h"
#include "McuXFormat.h"
#include "gui.h"
#include "toaster.h"

static lv_obj_t *win;
static lv_obj_t *chart_label;
static lv_obj_t *chart;
static lv_chart_series_t *charge_ser, *voltage_ser;
#define CHART_MAX_VALUE   100
#define CHART_POINT_NUM   100
#if LV_COLOR_DEPTH==1
  #define CHARGE_LABEL_COLOR   "000000"
  #define VOLTAGE_LABEL_COLOR      "000000"
#else
  #define TEMPERATURE_LABEL_COLOR   "FF0000"
  #define HUMIDITY_LABEL_COLOR      "00FF00"
#endif
static lv_task_t *refr_task;
#define REFR_TIME_MS   (1000)

/**
 * Called when the window's close button is clicked
 * @param btn pointer to the close button
 * @return LV_ACTION_RES_INV because the window is deleted in the function
 */
static lv_res_t win_close_action(lv_obj_t *btn) {
  GUI_GroupPull();
  lv_obj_del(win);
  win = NULL;
  lv_task_del(refr_task);
  refr_task = NULL;
  return LV_RES_INV;
}

/**
 * Called periodically to monitor the LED.
 * @param param unused
 */
static void refresh_task(void *param) {
  unsigned char buf[48];
  float charge, voltage;
  int16_t chart_vvalue;
  int16_t chart_cvalue;

  if (TOASTER_IsRunning()) {
    return;
  }

  if (UPS_GetVoltage(&voltage)!=0) {
    voltage = 0.0f; /* error */
  }
  if (UPS_GetCharge(&charge)!=0) {
    charge = 0.0f; /* error */
  }
  chart_vvalue = voltage+30; /* add offset */
  if (chart_vvalue>CHART_MAX_VALUE) {
    chart_vvalue = CHART_MAX_VALUE;
  } else if (chart_vvalue<0) {
    chart_vvalue = 0;
  }
  chart_cvalue = charge;
  if (chart_cvalue>CHART_MAX_VALUE) {
    chart_cvalue = CHART_MAX_VALUE;
  } else if (chart_cvalue<0) {
    chart_cvalue = 0;
  }
  /*Add the data to the chart*/
  lv_chart_set_next(chart, charge_ser, chart_cvalue);
  lv_chart_set_next(chart, voltage_ser, chart_vvalue);

  McuXFormat_xsnprintf((char*)buf, sizeof(buf), "%s%s V: %.1fV%s\n%s%s Ch: %.1f%%%s\ncharge: %c",
    LV_TXT_COLOR_CMD, VOLTAGE_LABEL_COLOR, voltage, LV_TXT_COLOR_CMD,
    LV_TXT_COLOR_CMD, CHARGE_LABEL_COLOR,  charge, LV_TXT_COLOR_CMD,
    UPS_IsCharging()?'y':'n'
    );
  lv_label_set_text(chart_label, (char*)buf);
}

void GUI_UPS_CreateView(void) {
    lv_obj_t *closeBtn;
    refr_task = lv_task_create(refresh_task, REFR_TIME_MS, LV_TASK_PRIO_LOW, NULL);

    win = lv_win_create(lv_scr_act(), NULL);
    lv_win_set_title(win, "UPS");
    closeBtn = lv_win_add_btn(win, SYMBOL_CLOSE, win_close_action);
    GUI_GroupPush();
    GUI_AddObjToGroup(closeBtn);
    lv_group_focus_obj(closeBtn);
    /* Make the window content responsive */
    lv_win_set_layout(win, LV_LAYOUT_PRETTY);

    /* Create a chart with two data lines */
    chart = lv_chart_create(win, NULL);
    lv_obj_set_size(chart, LV_HOR_RES/2, LV_VER_RES/2);

    lv_obj_set_pos(chart, LV_DPI/10, LV_DPI/10);

    lv_chart_set_point_count(chart, CHART_POINT_NUM);
    lv_chart_set_range(chart, 0, CHART_MAX_VALUE);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_series_width(chart, 3);
    voltage_ser =  lv_chart_add_series(chart, /*LV_COLOR_RED*/LV_COLOR_BLACK);
    charge_ser =  lv_chart_add_series(chart, /*LV_COLOR_GREEN*/LV_COLOR_BLACK);

    /* Set the data series to zero */
    uint16_t i;
    for(i = 0; i<CHART_POINT_NUM; i++) {
        lv_chart_set_next(chart, voltage_ser, 0);
        lv_chart_set_next(chart, charge_ser, 0);
    }
    /* label for values: */
    chart_label = lv_label_create(win, NULL);
    lv_label_set_recolor(chart_label, true);
    lv_obj_align(chart_label, chart, LV_ALIGN_OUT_RIGHT_TOP, LV_DPI/5, 0);

    /* Refresh the chart and label manually at first */
    refresh_task(NULL);
}
#endif /* PL_CONFIG_USE_UPS */
