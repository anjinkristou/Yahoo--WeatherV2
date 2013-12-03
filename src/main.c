//Declare and Import references
#include "pebble.h"
#include "pebble_fonts.h"


/*
//Watchface header section
#define MY_UUID { 0x6B, 0xE8, 0x63, 0x5E, 0x6D, 0x8D, 0x49, 0xA0, 0xA1, 0xA3, 0x38, 0x22, 0x77, 0x9D, 0x2C, 0x68 }


PBL_APP_INFO(MY_UUID,
             "Yahoo! Weather", "dabdemon", 
             1, 0, // App version
             RESOURCE_ID_IMAGE_MENU_ICON,
             APP_INFO_WATCH_FACE);
*/		 

#define WEEKDAY_FRAME	  (GRect(1,  2, 90, 168-145)) 
#define BATT_FRAME 	      (GRect(95,  4, 35, 168-146)) 
#define BT_FRAME 	      (GRect(120,  4, 20, 168-146)) 
#define TIME_FRAME        (GRect(0, 15, 144, 168-16)) 
#define DATE_FRAME        (GRect(1, 69, 139, 168-62)) 
/*
#define MAX_FRAME (GRect(65, 90, 40, 168-145))
#define MIN_FRAME (GRect(105, 90, 40, 168-145))
*/
	
#define LAST_UPDATE_FRAME (GRect(110,  148, 34, 168-145))
#define LOCATION_FRAME    (GRect(1,  148, 110, 168-145))
#define WEATHER_FRAME     (GRect(5,  90, 65, 168-108))
#define TEMPERATURE_FRAME (GRect(65, 95, 82, 168-118))
	

	
//******************//
// DEFINE THE ICONS //
//******************//	
static int LAYER_ICONS[] = {
	RESOURCE_ID_BT_CONNECTED,
	RESOURCE_ID_BT_DISCONNECTED,
};

static const uint32_t WEATHER_ICONS[] = {
  RESOURCE_ID_ICON_CLEAR_DAY,
  RESOURCE_ID_ICON_CLEAR_NIGHT,
  RESOURCE_ID_ICON_WIND,
  RESOURCE_ID_ICON_COLD,
  RESOURCE_ID_ICON_HOT,
  RESOURCE_ID_ICON_PARTLY_CLOUDY_DAY,
  RESOURCE_ID_ICON_PARTLY_CLOUDY_NIGHT,
  RESOURCE_ID_ICON_FOG,
  RESOURCE_ID_ICON_RAIN,
  RESOURCE_ID_ICON_SNOW,
  RESOURCE_ID_ICON_SLEET,
  RESOURCE_ID_ICON_SNOW_SLEET,
  RESOURCE_ID_ICON_RAIN_SLEET,
  RESOURCE_ID_ICON_RAIN_SNOW,
  RESOURCE_ID_ICON_CLOUDY,
  RESOURCE_ID_ICON_THUNDER,
  RESOURCE_ID_ICON_NOT_AVAILABLE,
  RESOURCE_ID_ICON_DRIZZLE,
};

//*************//
// Define KEYS //
//*************//

enum WeatherKey {
  WEATHER_ICON_KEY = 0x0,        	// TUPLE_INT
  WEATHER_TEMPERATURE_KEY = 0x1, 	// TUPLE_CSTRING
  WEATHER_CITY_KEY = 0x2,			//TUPLE_CSTRING
	/*
  WEATHER_MAX_KEY = 0x3,			//TUPLE_CSTRING
  WEATHER_MIN_KEY = 0x4,			//TUPLE_CSTRINGç
  */
};

//Declare initial window	
	Window *my_window;    

//Define the layers
	TextLayer *date_layer;   		// Layer for the date
	TextLayer *Time_Layer; 			// Layer for the time
	TextLayer *Weekday_Layer; 		//Layer for the weekday
	TextLayer *Last_Update; 		// Layer for the last update
	TextLayer *Location_Layer; 		// Layer for the last update
	TextLayer *Batt_Layer;			//Layer for the BT connection
	TextLayer *BT_Layer;			//Layer for the BT connection
	TextLayer *Temperature_Layer;	//Layer for the Temperature

/*
	TextLayer *Max_Layer;			//Layer for the Max Temperature
	TextLayer *Min_Layer;			//Layer for the Min Temperature
	*/
	static GBitmap *BT_image;
	static BitmapLayer *BT_icon_layer; //Layer for the BT connection
	
	static GBitmap *Batt_image;
	static BitmapLayer *Batt_icon_layer; //Layer for the Battery status
	
	static GBitmap *weather_image;
	static BitmapLayer *weather_icon_layer; //Layer for the weather info


//Define and initialize variables
	//FONTS
	GFont font_date;        // Font for date
	GFont font_time;        // Font for time
	GFont font_update;      // Font for last update
	GFont font_temperature;	// Font for the temperature

	//Vibe Control
	bool BTConnected = true;

	//Date & Time	
	static char last_update[]="00:00 ";
	static int initial_minute;

	static char weekday_text[] = "XXXXXXXXXX";
	static char date_text[] = "XXX 00";
	static char month_text[] = "XXXXXXXXXXXXX";
	static char day_text[] = "31";
	static char day_month[]= "31 SEPTEMBER"; 
	static char time_text[] = "00:00"; 
	
	bool translate_sp = true;

//*****************//
// AppSync options //
//*****************//

	static AppSync sync;
	static uint8_t sync_buffer[64];

static void sync_tuple_changed_callback(const uint32_t key,
                                        const Tuple* new_tuple,
                                        const Tuple* old_tuple,
                                        void* context) {

	
  // App Sync keeps new_tuple in sync_buffer, so we may use it directly
  switch (key) {
    case WEATHER_ICON_KEY:
      if (weather_image) {
        gbitmap_destroy(weather_image);
      }

      weather_image = gbitmap_create_with_resource(
          WEATHER_ICONS[new_tuple->value->uint8]);
      bitmap_layer_set_bitmap(weather_icon_layer, weather_image);
      break;

    case WEATHER_TEMPERATURE_KEY:
	  //Update the temperature
      text_layer_set_text(Temperature_Layer, new_tuple->value->cstring);
	  //Set the time on which weather was retrived
	  memcpy(&last_update, time_text, strlen(time_text));
	  text_layer_set_text(Last_Update, last_update);
      break;

	 //try to get city
	 case WEATHER_CITY_KEY:
	  text_layer_set_text(Location_Layer, new_tuple->value->cstring);
	  break;
	  /*
	  	 //try to get max
	case WEATHER_MAX_KEY:
	  text_layer_set_text(Max_Layer, new_tuple->value->cstring);
	  break;
	  
	case WEATHER_MIN_KEY:
	  text_layer_set_text(Min_Layer, new_tuple->value->cstring);
	  break;
  */
  }
}

//**************************//
// Check the Battery Status //
//**************************//

static void handle_battery(BatteryChargeState charge_state) {
  	static char battery_text[] = "100%";

  if (charge_state.is_charging) {
    //snprintf(battery_text, sizeof(battery_text), "charging");
	  Batt_image = gbitmap_create_with_resource(RESOURCE_ID_BATT_CHAR);
	  bitmap_layer_set_bitmap(Batt_icon_layer, Batt_image);
  } else {
    snprintf(battery_text, sizeof(battery_text), "%d%%", charge_state.charge_percent);
	  //if ((battery_text[0] == "1" || battery_text[0] == "2")  && strlen(battery_text)<4) //If the charge is between 0% and 20%
	  if (charge_state.charge_percent<20) //If the charge is between 0% and 20%
	  {
		Batt_image = gbitmap_create_with_resource(RESOURCE_ID_BATT_EMPTY);
	  	bitmap_layer_set_bitmap(Batt_icon_layer, Batt_image);
	  }
	  else if (charge_state.charge_percent<40)//If the charge is between 20% and 40%
	  {
		Batt_image = gbitmap_create_with_resource(RESOURCE_ID_BATT_20);
	  	bitmap_layer_set_bitmap(Batt_icon_layer, Batt_image);
	  }
	  else if (charge_state.charge_percent<60) //If the charge is between 40% and 60%
	  {
		Batt_image = gbitmap_create_with_resource(RESOURCE_ID_BATT_40);
	  	bitmap_layer_set_bitmap(Batt_icon_layer, Batt_image);
	  }
	  else if (charge_state.charge_percent<80) //If the charge is between 60% and 80%
	  {
		Batt_image = gbitmap_create_with_resource(RESOURCE_ID_BATT_60);
	  	bitmap_layer_set_bitmap(Batt_icon_layer, Batt_image);
	  }
	  else //IF the charge is between 80% and 100%
	  {
		Batt_image = gbitmap_create_with_resource(RESOURCE_ID_BATT_FULL);
	  	bitmap_layer_set_bitmap(Batt_icon_layer, Batt_image);
	  }
  }
  //text_layer_set_text(Batt_Layer, battery_text);
}

//******************************//
// Handle Bluetooth Connection  //
//*****************************//
static void handle_bluetooth(bool connected) 
{
  	//text_layer_set_text(BT_Layer, connected ? "C" : "D");
	
	//draw the BT icon if connected
	
	if(connected ==true)
	{
		BT_image = gbitmap_create_with_resource(RESOURCE_ID_BT_CONNECTED);
  		bitmap_layer_set_bitmap(BT_icon_layer, BT_image);
		if (BTConnected == false){
			//Vibes to alert connection
			vibes_double_pulse();
			BTConnected = true;
		}
	}
	else
	{
		BT_image = gbitmap_create_with_resource(RESOURCE_ID_BT_DISCONNECTED);
  		bitmap_layer_set_bitmap(BT_icon_layer, BT_image);
		if (BTConnected == true){
			//Vibes to alert disconnection
			vibes_long_pulse();
			BTConnected = false;
		}
	
	}
	
	
} //handle_bluetooth


void TranslateDate(){
			
			if (month_text[0] == 'J' && month_text[1] == 'a')
			{
				memcpy(&month_text, "   enero", strlen("   enero")+1); // January
			}
			
			if (month_text[0] == 'F' && month_text[1] == 'e')
			{
				memcpy(&month_text, "   febrero", strlen("   febrero")+1); // Febrary
			}
			
			if (month_text[0] == 'M' && month_text[2] == 'r')
			{
				memcpy(&month_text, "   marzo", strlen("   marzo")+1); // March
			}
			
			if (month_text[0] == 'A' && month_text[1] == 'p')
			{
				memcpy(&month_text, "   abril", strlen("   abril")+1); // April
			}
			
			if (month_text[0] == 'M' && month_text[2] == 'y')
			{
				memcpy(&month_text, "   de mayo", strlen("   de mayo")+1); // May
			}
			
			if (month_text[0] == 'J' && month_text[2] == 'n')
			{
				memcpy(&month_text, "   junio", strlen("   junio")+1); // June
			}
			
			if (month_text[0] == 'J' && month_text[2] == 'l')
			{
				memcpy(&month_text, "   julio", strlen("   julio")+1); // July
			}
			
			if (month_text[0] == 'A' && month_text[1] == 'u')
			{
				memcpy(&month_text, "   agosto ", strlen("   agosto ")+1); // August
			}
			
			if (month_text[0] == 'S' && month_text[1] == 'e')
			{
				memcpy(&month_text, "   septiembre", strlen("   septiembre")+1); // September
			}
			
			if (month_text[0] == 'O' && month_text[1] == 'c')
			{
				memcpy(&month_text, "   octubre", strlen("   octubre")+1); // October
			}
			
			if (month_text[0] == 'N' && month_text[1] == 'o')
			{
				memcpy(&month_text, "   noviembre", strlen("   noviembre")+1); // November
			}
			
			if (month_text[0] == 'D' && month_text[1] == 'e')
			{
				memcpy(&month_text, "   diciembre", strlen("   diciembre")+1); // December
			}
			
			// Primitive hack to translate the day of week to another language
			// Needs to be exactly 3 characters, e.g. "Mon" or "Mo "
			// Supported characters: A-Z, a-z, 0-9
			
			if (weekday_text[0] == 'M')
			{
				memcpy(&weekday_text, " Lunes", strlen(" Lunes")+1); // Monday
			}
			
			if (weekday_text[0] == 'T' && weekday_text[1] == 'u')
			{
				memcpy(&weekday_text, " Martes", strlen(" Martes")+1); // Tuesday
			}
			
			if (weekday_text[0] == 'W')
			{
				memcpy(&weekday_text, " Miercoles", strlen(" Miercoles")+1); // Wednesday
			}
			
			if (weekday_text[0] == 'T' && weekday_text[1] == 'h')
			{
				memcpy(&weekday_text, " Jueves", strlen(" Jueves")+1); // Thursday
			}
			
			if (weekday_text[0] == 'F')
			{
				memcpy(&weekday_text, " Viernes", strlen(" Viernes")+1); // Friday
			}
			
			if (weekday_text[0] == 'S' && weekday_text[1] == 'a')
			{
				memcpy(&weekday_text, " Sabado", strlen(" Sabado")+1); // Saturday
			}
			
			if (weekday_text[0] == 'S' && weekday_text[1] == 'u')
			{
				memcpy(&weekday_text, " Domingo", strlen(" Domingo")+1); // Sunday
			}
			

}



//************************//
// Capture the Tick event //
//************************//
void handle_tick(struct tm *tick_time, TimeUnits units_changed)
{

//Init the date
	
				//Get the Weekday
				strftime(weekday_text,sizeof(weekday_text),"%A",tick_time);
				//Get the Month + Day (English format)
				 strftime(month_text,sizeof(month_text),"%B %e",tick_time);
				//Get the Day + Month (Spanish format)
				strftime(day_month,sizeof(day_month),"%e %B",tick_time);


				if(translate_sp){
					//Get the Month
					strftime(month_text,sizeof(month_text),"%B",tick_time);
					//Get the day
					strftime(day_text,sizeof(day_text),"%e",tick_time);
					//Translate to Spanish
					TranslateDate();
					
					//Concatenate the day to the month
					memcpy(&month_text, day_text, strlen(day_text));
				}

						
				text_layer_set_text(date_layer, month_text);
				text_layer_set_text(Weekday_Layer, weekday_text); //Update the weekday layer	
				

	if (units_changed & MINUTE_UNIT) 
	{

			/*
			if (units_changed & DAY_UNIT)
			{	
				//Get the Weekday
				strftime(weekday_text,sizeof(weekday_text),"%A",tick_time);
				//Get the Month + Day (English format)
				 strftime(month_text,sizeof(month_text),"%B %d",tick_time);
				//Get the Day + Month (Spanish format)
				strftime(day_month,sizeof(day_month),"%d %B",tick_time);
				//Get the Month
			  	strftime(month_text,sizeof(month_text),"%B",tick_time);
				//Get the day
			  	strftime(day_text,sizeof(day_text),"%d",tick_time);

				//Translate to Spanish
				TranslateDate();
			
				//Concatenate the day to the month
				memcpy(&month_text, day_text, strlen(day_text));
				
				text_layer_set_text(date_layer, month_text);
				text_layer_set_text(Weekday_Layer, weekday_text); //Update the weekday layer
			} // DAY CHANGES
			*/

			//Format the time	
			if (clock_is_24h_style())
			{
				strftime(time_text, sizeof(time_text), "%H:%M", tick_time);
			}
			else
			{
				strftime(time_text, sizeof(time_text), "%I:%M", tick_time);
			}
		
	  			
  			text_layer_set_text(Time_Layer, time_text);
		
			//Check Battery Status
			handle_battery(battery_state_service_peek());
		
			//Check BT Status
			handle_bluetooth(bluetooth_connection_service_peek());

	} //MINUTE CHANGES
} //HANDLE_TICK 



//****************************//
// Initialize the application //
//****************************//

void handle_init(void)
{
	//Define Resources
    ResHandle res_d;
	ResHandle res_u;
	ResHandle res_t;
	ResHandle res_temp;
	
	//Create the main window
	my_window = window_create(); 
	window_stack_push(my_window, true /* Animated */);
	window_set_background_color(my_window, GColorBlack);
	
	
	//Load the custom fonts
	res_t = resource_get_handle(RESOURCE_ID_FUTURA_CONDENSED_53); // Time font
	res_d = resource_get_handle(RESOURCE_ID_FUTURA_18); // Date font
	res_u = resource_get_handle(RESOURCE_ID_FUTURA_10); // Last Update font
	//res_temp =  resource_get_handle(RESOURCE_ID_FUTURA_36); //Temperature
	
		
    font_date = fonts_load_custom_font(res_d);
	font_update = fonts_load_custom_font(res_u);
	font_time = fonts_load_custom_font(res_t);
	//font_temperature = fonts_load_custom_font(res_temp);
	font_temperature = fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT);
	
	
	//LOAD THE LAYERS
		//Display the Weekday layer
		Weekday_Layer = text_layer_create(WEEKDAY_FRAME);
		text_layer_set_text_color(Weekday_Layer, GColorWhite);
		text_layer_set_background_color(Weekday_Layer, GColorClear);
		text_layer_set_font(Weekday_Layer, font_date);
		text_layer_set_text_alignment(Weekday_Layer, GTextAlignmentLeft);
		layer_add_child(window_get_root_layer(my_window), text_layer_get_layer(Weekday_Layer)); 
	
		//Display the Batt layer
		Batt_icon_layer = bitmap_layer_create(BATT_FRAME);
  		bitmap_layer_set_bitmap(Batt_icon_layer, Batt_image);
  		layer_add_child(window_get_root_layer(my_window), bitmap_layer_get_layer(Batt_icon_layer));
	
		//Display the BT layer
	  	BT_icon_layer = bitmap_layer_create(BT_FRAME);
  		bitmap_layer_set_bitmap(BT_icon_layer, BT_image);
  		layer_add_child(window_get_root_layer(my_window), bitmap_layer_get_layer(BT_icon_layer));
	
		//Display the Time layer
		Time_Layer = text_layer_create(TIME_FRAME);
		text_layer_set_text_color(Time_Layer, GColorWhite);
		text_layer_set_background_color(Time_Layer, GColorClear);
		text_layer_set_font(Time_Layer, font_time);
		text_layer_set_text_alignment(Time_Layer, GTextAlignmentCenter);
		layer_add_child(window_get_root_layer(my_window), text_layer_get_layer(Time_Layer)); 
	
		//Display the Date layer
		date_layer = text_layer_create(DATE_FRAME);
		text_layer_set_text_color(date_layer, GColorWhite);
		text_layer_set_background_color(date_layer, GColorClear);
		text_layer_set_font(date_layer, font_date);
		text_layer_set_text_alignment(date_layer, GTextAlignmentRight);
		layer_add_child(window_get_root_layer(my_window), text_layer_get_layer(date_layer)); 
	
		//Display the Weather layer
		weather_icon_layer = bitmap_layer_create(WEATHER_FRAME);
  		bitmap_layer_set_bitmap(weather_icon_layer, weather_image);
  		layer_add_child(window_get_root_layer(my_window), bitmap_layer_get_layer(weather_icon_layer));
	
		//Display the Temperature layer
		Temperature_Layer = text_layer_create(TEMPERATURE_FRAME);
		text_layer_set_text_color(Temperature_Layer, GColorWhite);
		text_layer_set_background_color(Temperature_Layer, GColorClear);
		text_layer_set_font(Temperature_Layer, font_temperature);
		text_layer_set_text_alignment(Temperature_Layer, GTextAlignmentCenter);
		layer_add_child(window_get_root_layer(my_window), text_layer_get_layer(Temperature_Layer)); 
	
		//Display the Location layer
		Location_Layer = text_layer_create(LOCATION_FRAME);
		text_layer_set_text_color(Location_Layer, GColorWhite);
		text_layer_set_background_color(Location_Layer, GColorClear);
		text_layer_set_font(Location_Layer, font_update);
		text_layer_set_text_alignment(Location_Layer, GTextAlignmentRight);
		layer_add_child(window_get_root_layer(my_window), text_layer_get_layer(Location_Layer)); 
	
		//Display the Last Update layer
		Last_Update = text_layer_create(LAST_UPDATE_FRAME);
		text_layer_set_text_color(Last_Update, GColorWhite);
		text_layer_set_background_color(Last_Update, GColorClear);
		text_layer_set_font(Last_Update, font_update);
		text_layer_set_text_alignment(Last_Update, GTextAlignmentRight);
		layer_add_child(window_get_root_layer(my_window), text_layer_get_layer(Last_Update)); 
	
	/*
			//Display the Max layer
		Max_Layer = text_layer_create(MAX_FRAME);
		text_layer_set_text_color(Max_Layer, GColorWhite);
		text_layer_set_background_color(Max_Layer, GColorClear);
		text_layer_set_font(Max_Layer, font_update);
		text_layer_set_text_alignment(Max_Layer, GTextAlignmentRight);
		layer_add_child(window_get_root_layer(my_window), text_layer_get_layer(Max_Layer)); 

			//Display the Min layer
		Min_Layer = text_layer_create(MIN_FRAME);
		text_layer_set_text_color(Min_Layer, GColorWhite);
		text_layer_set_background_color(Min_Layer, GColorClear);
		text_layer_set_font(Min_Layer, font_update);
		text_layer_set_text_alignment(Min_Layer, GTextAlignmentRight);
		layer_add_child(window_get_root_layer(my_window), text_layer_get_layer(Min_Layer)); 

*/
	
	 // Setup messaging
		const int inbound_size = 64;
		const int outbound_size = 64;
		app_message_open(inbound_size, outbound_size);
	
		Tuplet initial_values[] = {
		TupletInteger(WEATHER_ICON_KEY, (uint8_t) 16), //INITIALIZE TO "N/A"
		TupletCString(WEATHER_TEMPERATURE_KEY, ""),
		TupletCString(WEATHER_CITY_KEY, ""),
			/*
			TupletCString(WEATHER_MAX_KEY, ""),
			TupletCString(WEATHER_MIN_KEY, ""),
			*/
		}; //TUPLET INITIAL VALUES
	
	  	app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values,
                ARRAY_LENGTH(initial_values), sync_tuple_changed_callback,
                NULL, NULL);
	
	// Ensures time is displayed immediately (will break if NULL tick event accessed).
	  // (This is why it's a good idea to have a separate routine to do the update itself.)
	  	
		time_t now = time(NULL);
	  	struct tm *current_time = localtime(&now);
		handle_tick(current_time, MINUTE_UNIT);
		tick_timer_service_subscribe(MINUTE_UNIT, &handle_tick);
	
		//Enable the Battery check event
		battery_state_service_subscribe(&handle_battery);
		//Enable the Bluetooth check event
	 	bluetooth_connection_service_subscribe(&handle_bluetooth);

} //HANDLE_INIT



//**********************//
// Kill the application //
//**********************//
void handle_deinit(void)
{
  //text_layer_destroy(text_layer);

	//Unsuscribe services
	tick_timer_service_unsubscribe();
 	battery_state_service_unsubscribe();
  	bluetooth_connection_service_unsubscribe();
	
	if (BT_image){gbitmap_destroy(BT_image);}
	if (Batt_image){gbitmap_destroy(Batt_image);}
	if (weather_image){gbitmap_destroy(weather_image);}
	
	//Deallocate layers
	text_layer_destroy(Time_Layer);
	text_layer_destroy(date_layer);
	text_layer_destroy(Weekday_Layer);
	text_layer_destroy(Temperature_Layer);	
	text_layer_destroy(Location_Layer);	
	text_layer_destroy(Last_Update);	
	
	//Deallocate custom fonts
	fonts_unload_custom_font(font_date);
	fonts_unload_custom_font(font_update);
	fonts_unload_custom_font(font_time);
	
	//Deallocate the main window
  	window_destroy(my_window);

} //HANDLE_DEINIT


//*************//
// ENTRY POINT //
//*************//
int main(void) 
{	
	handle_init();
	app_event_loop();
	handle_deinit();
}