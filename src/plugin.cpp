#include <string.h>
#include <stdio.h>
#include <cstring>
#include <ctime>

#include "XPLMPlugin.h"
#include "XPLMMenus.h"
#include "XPLMDataAccess.h"
#include "XPLMProcessing.h"
#include "XPWidgets.h"
#include "XPStandardWidgets.h"
#include "XPWidgetDefs.h"

// Window dimensions
const int WINDOW_WIDTH = 130;
const int WINDOW_HEIGHT = 310;
const int WINDOW_LEFT = 100;
const int WINDOW_TOP = 600;
const int WINDOW_RIGHT = WINDOW_LEFT + WINDOW_WIDTH;
const int WINDOW_BOTTOM = WINDOW_TOP - WINDOW_HEIGHT;
const int RPM_LABEL_Y = WINDOW_TOP - 25;
const int THROTTLE_LABEL_Y = WINDOW_TOP - 45;
const int SLIDER_X = WINDOW_LEFT + 10;
const int SLIDER_Y_TOP = WINDOW_TOP - 70;     // Top of slider (closer to window top, larger Y)
const int SLIDER_Y_BOTTOM = WINDOW_TOP - 220; // Bottom of slider (further from window top, smaller Y)
const int SLIDER_WIDTH = 20;
const int PRESET_BUTTON_X = SLIDER_X + SLIDER_WIDTH + 15; // To the right of slider with more spacing
const int PRESET_BUTTON_WIDTH = 35;
const int PRESET_BUTTON_HEIGHT = 20;
const int PRESET_2400_Y = SLIDER_Y_TOP; // Top preset button
const int PRESET_1000_Y = PRESET_2400_Y - PRESET_BUTTON_HEIGHT - 5; // Stacked under 2400 button with more space
const int SLIDER_VALUE_LABEL_Y = WINDOW_TOP - 230;
const int CHECKBOX_Y = WINDOW_TOP - 250;
const int BUTTON_Y = WINDOW_TOP - 275;

const char* DATAREF_ENGINE_RPM = "sim/cockpit2/engine/indicators/engine_speed_rpm";
const char* DATAREF_THROTTLE_POSITION = "sim/cockpit2/engine/actuators/throttle_ratio_all";

static XPWidgetID g_main_window = nullptr;
static XPWidgetID g_rpm_label = nullptr;
static XPWidgetID g_throttle_label = nullptr;
static XPWidgetID g_rpm_slider = nullptr;
static XPWidgetID g_slider_value_label = nullptr;
static XPWidgetID g_rpm_preset_2400 = nullptr;
static XPWidgetID g_rpm_preset_1000 = nullptr;
static XPWidgetID g_autothrottle_button = nullptr;
static XPWidgetID g_reload_button = nullptr;

static bool g_autothrottle_enabled = false;

static XPLMDataRef g_rpm_dataref = nullptr;
static XPLMDataRef g_throttle_dataref = nullptr;

// Autothrottle timing variables
static float g_total_elapsed_time = 0.0f;
static float g_last_throttle_adjust_time = 0.0f;
static float g_rpm_out_of_tolerance_start_time = -1.0f;

static int WidgetCallback(XPWidgetMessage inMessage, XPWidgetID inWidget, intptr_t inParam1, intptr_t inParam2);
static float FlightLoopCallback(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon);
static void UpdateDatarefHandles(void);
static void UpdateRpmLabel(void);
static void UpdateThrottleLabel(void);
static void UpdateSliderValueLabel(void);
static void UpdateAutothrottle(void);
static void CreatePopupWindow(void);
static void XPAutothrottleMenuHandler(void * mRef, void * iRef);

PLUGIN_API int XPluginStart(char* outName, char* outSignature, char* outDescription) {
    XPLMMenuID id;
    int item;

    strcpy(outName, "XPAutothrottle");
    strcpy(outSignature, "nz.m2.xpautothrottle");
    strcpy(outDescription, "X-Plane AutoThrottle plugin");

    // Enable native widget windows for proper DPI scaling
    // This makes widgets use modern XPLMDisplay windows that handle UI scaling correctly
    if (XPLMHasFeature("XPLM_USE_NATIVE_WIDGET_WINDOWS")) {
        XPLMEnableFeature("XPLM_USE_NATIVE_WIDGET_WINDOWS", 1);
    }

    item = XPLMAppendMenuItem(XPLMFindPluginsMenu(), "XPAutoThrottle", NULL, 1);
    id = XPLMCreateMenu("XPAutoThrottle", XPLMFindPluginsMenu(), item, XPAutothrottleMenuHandler, NULL);
    XPLMAppendMenuItem(id, "Show Window", (void *)"Show", 1);
    XPLMAppendMenuItem(id, "Hide Window", (void *)"Hide", 1);
    XPLMAppendMenuItem(id, "Reload plugins", (void *)"Reload", 1);

    return 1;
}

PLUGIN_API void XPluginStop(void) {
    if (g_main_window) {
        XPDestroyWidget(g_main_window, 1);
        g_main_window = nullptr;
        g_rpm_label = nullptr;
        g_throttle_label = nullptr;
        g_rpm_slider = nullptr;
        g_slider_value_label = nullptr;
        g_rpm_preset_2400 = nullptr;
        g_rpm_preset_1000 = nullptr;
        g_autothrottle_button = nullptr;
        g_reload_button = nullptr;
        g_rpm_dataref = nullptr;
        g_throttle_dataref = nullptr;
        g_autothrottle_enabled = false;
    }
}

PLUGIN_API int XPluginEnable(void) {
    g_rpm_dataref = XPLMFindDataRef(DATAREF_ENGINE_RPM);
    g_throttle_dataref = XPLMFindDataRef(DATAREF_THROTTLE_POSITION);
    
    if (!g_main_window) {
        CreatePopupWindow();
    } else {
        XPShowWidget(g_main_window);
    }
    
    XPLMRegisterFlightLoopCallback(FlightLoopCallback, 0.1f, nullptr);
    
    return 1;
}

PLUGIN_API void XPluginDisable(void) {
    XPLMUnregisterFlightLoopCallback(FlightLoopCallback, nullptr);
    
    if (g_main_window) {
        XPShowWidget(g_main_window);
    }
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFrom, int inMessage, void* inParam) {
    (void)inFrom;
    (void)inMessage;
    (void)inParam;
}

void XPAutothrottleMenuHandler(void * mRef, void * iRef) {
    (void)mRef;
    
    if (!strcmp((char *) iRef, "Show")) {
        if (!g_main_window) {
            CreatePopupWindow();
        } else {
            XPShowWidget(g_main_window);
        }
    } else if (!strcmp((char *) iRef, "Hide")) {
        if (g_main_window) {
            XPHideWidget(g_main_window);
        }
    } else if (!strcmp((char *) iRef, "Reload")) {
        XPLMReloadPlugins();
    }
}

static void CreatePopupWindow(void) {
    g_main_window = XPCreateWidget(
        WINDOW_LEFT, WINDOW_TOP, WINDOW_RIGHT, WINDOW_BOTTOM,
        1,
        "XPAutoThrottle",
        1,
        0,
        xpWidgetClass_MainWindow
    );
    
    if (g_main_window) {
        XPSetWidgetProperty(g_main_window, xpProperty_MainWindowHasCloseBoxes, 1);
        XPAddWidgetCallback(g_main_window, WidgetCallback);
        
        // Create RPM label
        g_rpm_label = XPCreateWidget(
            WINDOW_LEFT + 10, RPM_LABEL_Y, WINDOW_LEFT + WINDOW_WIDTH - 10, RPM_LABEL_Y - 20,
            1, "RPM: 0",
            0, g_main_window,
            xpWidgetClass_Caption
        );
        
        // Create Throttle label
        g_throttle_label = XPCreateWidget(
            WINDOW_LEFT + 10, THROTTLE_LABEL_Y, WINDOW_LEFT + WINDOW_WIDTH - 10, THROTTLE_LABEL_Y - 20,
            1, "Throttle: 0%",
            0, g_main_window,
            xpWidgetClass_Caption
        );
        
        // Create RPM slider (0 to 2500) - vertical slider
        // Vertical slider: narrow width (20px), tall height (150px)
        g_rpm_slider = XPCreateWidget(
            SLIDER_X, SLIDER_Y_TOP, SLIDER_X + 20, SLIDER_Y_BOTTOM,
            1, "",
            0, g_main_window,
            xpWidgetClass_ScrollBar
        );
        
        // Set slider properties
        XPSetWidgetProperty(g_rpm_slider, xpProperty_ScrollBarType, xpScrollBarTypeSlider);
        XPSetWidgetProperty(g_rpm_slider, xpProperty_ScrollBarMin, 0);
        XPSetWidgetProperty(g_rpm_slider, xpProperty_ScrollBarMax, 2500);
        XPSetWidgetProperty(g_rpm_slider, xpProperty_ScrollBarSliderPosition, 1000); // Default to 1000 RPM
        XPSetWidgetProperty(g_rpm_slider, xpProperty_ScrollBarPageAmount, 100);
        
        // Create slider value label to show current target RPM
        g_slider_value_label = XPCreateWidget(
            WINDOW_LEFT + 10, SLIDER_VALUE_LABEL_Y, WINDOW_LEFT + WINDOW_WIDTH - 10, SLIDER_VALUE_LABEL_Y - 15,
            1, "Target RPM: 1000",
            0, g_main_window,
            xpWidgetClass_Caption
        );
        
        // Create preset RPM buttons to the right of slider
        g_rpm_preset_2400 = XPCreateWidget(
            PRESET_BUTTON_X, PRESET_2400_Y, PRESET_BUTTON_X + PRESET_BUTTON_WIDTH, PRESET_2400_Y - PRESET_BUTTON_HEIGHT,
            1, "2400",
            0, g_main_window,
            xpWidgetClass_Button
        );
        XPSetWidgetProperty(g_rpm_preset_2400, xpProperty_ButtonType, xpPushButton);
        XPSetWidgetProperty(g_rpm_preset_2400, xpProperty_ButtonBehavior, xpButtonBehaviorPushButton);
        
        g_rpm_preset_1000 = XPCreateWidget(
            PRESET_BUTTON_X, PRESET_1000_Y, PRESET_BUTTON_X + PRESET_BUTTON_WIDTH, PRESET_1000_Y - PRESET_BUTTON_HEIGHT,
            1, "1000",
            0, g_main_window,
            xpWidgetClass_Button
        );
        XPSetWidgetProperty(g_rpm_preset_1000, xpProperty_ButtonType, xpPushButton);
        XPSetWidgetProperty(g_rpm_preset_1000, xpProperty_ButtonBehavior, xpButtonBehaviorPushButton);
        
        // Create autothrottle toggle button (ON/OFF) - same width as Reload button
        g_autothrottle_button = XPCreateWidget(
            WINDOW_LEFT + 10, CHECKBOX_Y, WINDOW_LEFT + 120, CHECKBOX_Y - 20,
            1, "OFF",
            0, g_main_window,
            xpWidgetClass_Button
        );
        
        // Set button properties
        XPSetWidgetProperty(g_autothrottle_button, xpProperty_ButtonType, xpPushButton);
        XPSetWidgetProperty(g_autothrottle_button, xpProperty_ButtonBehavior, xpButtonBehaviorPushButton);
        XPSetWidgetProperty(g_autothrottle_button, xpProperty_Hilited, 0); // Not hilited (grey) by default - OFF state
        
        g_reload_button = XPCreateWidget(
            WINDOW_LEFT + 10, BUTTON_Y, WINDOW_LEFT + 120, BUTTON_Y - 20,
            1, "Reload Plugins",
            0, g_main_window,
            xpWidgetClass_Button
        );
        
        XPSetWidgetProperty(g_reload_button, xpProperty_ButtonType, xpPushButton);
        XPSetWidgetProperty(g_reload_button, xpProperty_ButtonBehavior, xpButtonBehaviorPushButton);
    }
}

int WidgetCallback(XPWidgetMessage inMessage, XPWidgetID inWidget, intptr_t inParam1, intptr_t inParam2) {
    (void)inParam2;
    
    if (inMessage == xpMessage_CloseButtonPushed) {
        if (inWidget == g_main_window) {
            XPHideWidget(g_main_window);
            return 1;
        }
    }
    
    // Handle preset RPM button presses
    if (inMessage == xpMsg_PushButtonPressed) {
        if ((XPWidgetID)inParam1 == g_rpm_preset_2400) {
            // Set slider to 2400 RPM (already a 100 increment)
            XPSetWidgetProperty(g_rpm_slider, xpProperty_ScrollBarSliderPosition, 2400);
            // Update label immediately
            UpdateSliderValueLabel();
            return 1;
        }
        if ((XPWidgetID)inParam1 == g_rpm_preset_1000) {
            // Set slider to 1000 RPM (already a 100 increment)
            XPSetWidgetProperty(g_rpm_slider, xpProperty_ScrollBarSliderPosition, 1000);
            // Update label immediately
            UpdateSliderValueLabel();
            return 1;
        }
        if ((XPWidgetID)inParam1 == g_reload_button) {
            XPLMReloadPlugins();
            return 1;
        }
    }
    
    // Handle slider position change
    if (inMessage == xpMsg_ScrollBarSliderPositionChanged) {
        if ((XPWidgetID)inParam1 == g_rpm_slider) {
            // Get current slider value and snap to nearest 100
            int slider_value = (int)XPGetWidgetProperty(g_rpm_slider, xpProperty_ScrollBarSliderPosition, NULL);
            int snapped_value = ((slider_value + 50) / 100) * 100; // Round to nearest 100
            
            // Clamp to valid range (0-2500)
            if (snapped_value < 0) snapped_value = 0;
            if (snapped_value > 2500) snapped_value = 2500;
            
            // Update slider position to snapped value
            XPSetWidgetProperty(g_rpm_slider, xpProperty_ScrollBarSliderPosition, snapped_value);
            
            // Update slider value label
            char label_text[256];
            snprintf(label_text, sizeof(label_text), "Target RPM: %d", snapped_value);
            if (g_slider_value_label) {
                XPSetWidgetDescriptor(g_slider_value_label, label_text);
            }
            return 1;
        }
    }
    
    // Handle autothrottle button press
    if (inMessage == xpMsg_PushButtonPressed) {
        if ((XPWidgetID)inParam1 == g_autothrottle_button) {
            // Toggle autothrottle state
            g_autothrottle_enabled = !g_autothrottle_enabled;
            
            // Update button text and appearance
            if (g_autothrottle_enabled) {
                XPSetWidgetDescriptor(g_autothrottle_button, "ON");
                // Note: XPWidgets doesn't directly support color changes, but we can use text to indicate state
            } else {
                XPSetWidgetDescriptor(g_autothrottle_button, "OFF");
            }
            return 1;
        }
    }
    
    return 0;
}

// Flight loop callback to update RPM label
float FlightLoopCallback(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon) {
    (void)inElapsedTimeSinceLastFlightLoop;
    (void)inCounter;
    (void)inRefcon;

    g_total_elapsed_time += inElapsedSinceLastCall;
    
    UpdateRpmLabel();
    UpdateThrottleLabel();
    UpdateSliderValueLabel();
    UpdateAutothrottle();
    
    return 0.1f;
}

static void UpdateRpmLabel(void) {
    if (!g_rpm_label) {
        return;
    }
    
    // Read current RPM value
    float rpm_value = 0.0f;
    if (g_rpm_dataref) {
        XPLMDataTypeID rpm_type = XPLMGetDataRefTypes(g_rpm_dataref);
        if (rpm_type & xplmType_FloatArray) {
            float array_value[1];
            if (XPLMGetDatavf(g_rpm_dataref, array_value, 0, 1) > 0) {
                rpm_value = array_value[0];
            }
        } else if (rpm_type & xplmType_Float) {
            rpm_value = XPLMGetDataf(g_rpm_dataref);
        } else if (rpm_type & xplmType_Int) {
            rpm_value = (float)XPLMGetDatai(g_rpm_dataref);
        } else if (rpm_type & xplmType_IntArray) {
            int array_value[1];
            if (XPLMGetDatavi(g_rpm_dataref, array_value, 0, 1) > 0) {
                rpm_value = (float)array_value[0];
            }
        }
    }
    
    char rpm_text[256];
    if (g_rpm_dataref) {
        snprintf(rpm_text, sizeof(rpm_text), "RPM: %.0f", rpm_value);
    } else {
        snprintf(rpm_text, sizeof(rpm_text), "RPM: INVALID");
    }

    XPSetWidgetDescriptor(g_rpm_label, rpm_text);
}

static void UpdateThrottleLabel(void) {
    if (!g_throttle_label) {
        return;
    }
    
    // Read current throttle value
    float throttle_value = 0.0f;
    if (g_throttle_dataref) {
        XPLMDataTypeID throttle_type = XPLMGetDataRefTypes(g_throttle_dataref);
        if (throttle_type & xplmType_FloatArray) {
            float array_value[1];
            if (XPLMGetDatavf(g_throttle_dataref, array_value, 0, 1) > 0) {
                throttle_value = array_value[0];
            }
        } else {
            throttle_value = XPLMGetDataf(g_throttle_dataref);
        }
    }
    
    // Clamp throttle value to valid range (0.0-1.0) and convert to percentage
    if (throttle_value < 0.0f) throttle_value = 0.0f;
    if (throttle_value > 1.0f) throttle_value = 1.0f;
    float throttle_percent = throttle_value * 100.0f; // Convert to percentage
    
    char throttle_text[256];
    if (g_throttle_dataref) {
        snprintf(throttle_text, sizeof(throttle_text), "Throttle: %.1f%%", throttle_percent);
    } else {
        snprintf(throttle_text, sizeof(throttle_text), "Throttle: INVALID");
    }

    XPSetWidgetDescriptor(g_throttle_label, throttle_text);
}

// Update slider value label to show current target RPM
static void UpdateSliderValueLabel(void) {
    if (!g_slider_value_label || !g_rpm_slider) {
        return;
    }
    
    int slider_value = (int)XPGetWidgetProperty(g_rpm_slider, xpProperty_ScrollBarSliderPosition, NULL);
    char label_text[256];
    snprintf(label_text, sizeof(label_text), "Target RPM: %d", slider_value);
    XPSetWidgetDescriptor(g_slider_value_label, label_text);
}

// Autothrottle function: adjusts throttle to maintain target RPM
static void UpdateAutothrottle(void) {
    // Check if autothrottle is enabled
    if (!g_autothrottle_enabled) {
        g_rpm_out_of_tolerance_start_time = -1.0f; // Reset timing when disabled
        return;
    }
    
    // Check if we have all required datarefs and widgets
    if (!g_rpm_dataref || !g_throttle_dataref || !g_rpm_slider) {
        return;
    }
    
    // Get target RPM from slider
    int target_rpm = (int)XPGetWidgetProperty(g_rpm_slider, xpProperty_ScrollBarSliderPosition, NULL);
    
    // Get current RPM from dataref
    float current_rpm = 0.0f;
    XPLMDataTypeID rpm_type = XPLMGetDataRefTypes(g_rpm_dataref);
    if (rpm_type & xplmType_FloatArray) {
        float array_value[1];
        if (XPLMGetDatavf(g_rpm_dataref, array_value, 0, 1) > 0) {
            current_rpm = array_value[0];
        }
    } else if (rpm_type & xplmType_Float) {
        current_rpm = XPLMGetDataf(g_rpm_dataref);
    } else if (rpm_type & xplmType_Int) {
        current_rpm = (float)XPLMGetDatai(g_rpm_dataref);
    } else if (rpm_type & xplmType_IntArray) {
        int array_value[1];
        if (XPLMGetDatavi(g_rpm_dataref, array_value, 0, 1) > 0) {
            current_rpm = (float)array_value[0];
        }
    }
    
    // Get current throttle position
    float current_throttle = 0.0f;
    XPLMDataTypeID throttle_type = XPLMGetDataRefTypes(g_throttle_dataref);
    if (throttle_type & xplmType_FloatArray) {
        float array_value[1];
        if (XPLMGetDatavf(g_throttle_dataref, array_value, 0, 1) > 0) {
            current_throttle = array_value[0];
        }
    } else {
        current_throttle = XPLMGetDataf(g_throttle_dataref);
    }
   
    float rpm_diff = (float)target_rpm - current_rpm;
    
    const float RPM_TOLERANCE = 15.0f; // Keep it within 15 RPM of the target RPM
    const float THROTTLE_ADJUSTMENT = 0.001f; // Adjust by 0.5% for every 100 RPM off target
    const float SETTLE_TIME = 2.0f; // Wait 2 seconds before any adjustment
    const float MIN_ADJUST_INTERVAL = 1.0f; // Only adjust once per second
    
    if (rpm_diff > RPM_TOLERANCE || rpm_diff < -RPM_TOLERANCE) {
        if (g_rpm_out_of_tolerance_start_time < 0.0f) {
            g_rpm_out_of_tolerance_start_time = g_total_elapsed_time;
        }
        
        float time_out_of_tolerance = g_total_elapsed_time - g_rpm_out_of_tolerance_start_time;
        float time_since_last_adjust = g_total_elapsed_time - g_last_throttle_adjust_time;
        
        if (time_out_of_tolerance >= SETTLE_TIME && time_since_last_adjust >= MIN_ADJUST_INTERVAL) {
            float abs_rpm_diff = (rpm_diff > 0.0f) ? rpm_diff : -rpm_diff;
            int hundred_rpm_units = (int)(abs_rpm_diff / 100.0f);
            float dynamic_adjustment = THROTTLE_ADJUSTMENT;
            
            for (int i = 0; i < hundred_rpm_units; i++) {
                dynamic_adjustment *= 2.0f;
            }

            const float MAX_ADJUSTMENT = 0.1f;
            if (dynamic_adjustment > MAX_ADJUSTMENT) {
                dynamic_adjustment = MAX_ADJUSTMENT;
            }

            float new_throttle = current_throttle;
            
            if (rpm_diff > RPM_TOLERANCE) {
                new_throttle = current_throttle + dynamic_adjustment;
                if (new_throttle > 1.0f) {
                    new_throttle = 1.0f;
                }
            } else if (rpm_diff < -RPM_TOLERANCE) {
                new_throttle = current_throttle - dynamic_adjustment;
                if (new_throttle < 0.0f) {
                    new_throttle = 0.0f;
                }
            }
            
            if (new_throttle != current_throttle) {
                XPLMDataTypeID throttle_write_type = XPLMGetDataRefTypes(g_throttle_dataref);
                if (throttle_write_type & xplmType_FloatArray) {
                    float array_value[1] = { new_throttle };
                    XPLMSetDatavf(g_throttle_dataref, array_value, 0, 1);
                } else {
                    XPLMSetDataf(g_throttle_dataref, new_throttle);
                }
                g_last_throttle_adjust_time = g_total_elapsed_time;
            }
        }
    } else {
        g_rpm_out_of_tolerance_start_time = -1.0f;
    }
}
