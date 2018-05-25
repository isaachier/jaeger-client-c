include(hunter_cacheable)
include(hunter_download)
include(hunter_pick_scheme)

hunter_pick_scheme(DEFAULT url_sha1_cmake)
hunter_cacheable("opentracing-c")
hunter_download(PACKAGE_NAME "opentracing-c")
