
configure_file(wudb.py wudb.py ESCAPE_QUOTES @ONLY)

install(FILES cadocommand.py DESTINATION ${LIBSUFFIX}/scripts/cadofactor)
install(FILES cadologger.py DESTINATION ${LIBSUFFIX}/scripts/cadofactor)
install(FILES cadoparams.py DESTINATION ${LIBSUFFIX}/scripts/cadofactor)
install(FILES cadoprograms.py DESTINATION ${LIBSUFFIX}/scripts/cadofactor)
install(FILES cadotask.py DESTINATION ${LIBSUFFIX}/scripts/cadofactor)
install(FILES patterns.py DESTINATION ${LIBSUFFIX}/scripts/cadofactor)
# install(FILES signalhandler.py DESTINATION ${LIBSUFFIX}/scripts/cadofactor)
install(FILES upload.py DESTINATION ${LIBSUFFIX}/scripts/cadofactor)
# install(FILES verify.py DESTINATION ${LIBSUFFIX}/scripts/cadofactor)
install(FILES workunit.py DESTINATION ${LIBSUFFIX}/scripts/cadofactor)
install(FILES toplevel.py DESTINATION ${LIBSUFFIX}/scripts/cadofactor)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/wudb.py DESTINATION ${LIBSUFFIX}/scripts/cadofactor)
install(FILES wuserver.py DESTINATION ${LIBSUFFIX}/scripts/cadofactor)
