set(srcs "./app.cpp"
   "./appconfig.cpp"
  "./corncon23.cpp"
  "./vkeyboard.cpp"
  "./timezones.cpp"
  "./menus/menu_state.cpp"
  "./menus/appbase_menu.cpp"
  "./menus/gui_list_processor.cpp"
  "./menus/setting_menu.cpp"
  "./menus/menu3d.cpp"
  "./menus/3d/renderer.cpp"
  "./menus/game_of_life.cpp"
  "./menus/badge_test.cpp"
  "./menus/wifi_menu.cpp"
  "./menus/connection_details.cpp"
  "./menus/update_menu.cpp"
  "./menus/simon_says.cpp"
  "./menus/simon_says_multi.cpp"
  "./menus/sleep_menu.cpp"
)

idf_component_register(SRCS "${srcs}"
   INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../components/esp-etl/etl/include/ ../components/libesp
		)



