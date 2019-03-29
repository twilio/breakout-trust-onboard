cmake_minimum_required(VERSION 3.7)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/service/seeed_lte_hat.py DESTINATION /usr/local/seeed_lte_hat)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/service/seeed_lte_hat.service DESTINATION /etc/systemd/system)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/service/twilio_ppp.service DESTINATION /etc/systemd/system)


set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/debian/postinst;" PARENT_SCOPE)

