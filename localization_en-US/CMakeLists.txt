# resource language and its ID
# for a list of languages and IDs, visit https://msdn.microsoft.com/en-us/library/hh825678.aspx
set(language en-US)
set(language_id 0x0409)

set(localization_resource_header  ${CMAKE_CURRENT_LIST_DIR}/resource.h)
set(localization_resource ${CMAKE_CURRENT_LIST_DIR}/THRotator_en-US.rc)

include(../internationalization/THRotator_i18n.cmake)
