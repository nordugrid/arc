from acix.cacheserver import cachesetup

## cache dir will by "guessed" from parsing /etc/arc.conf
application = cachesetup.createCacheApplication()


## set cache dir manually example
# CACHE_DIRS = ["/user/grid/cache"]
# application = cachesetup.createCacheApplication(cache_dir=CACHE_DIRS)

## higher capacity example
# application = cachesetup.createCacheApplication(cache_dir=CACHE_DIRS, capacity=50000)

## custom port example
# application = cachesetup.createCacheApplication(cache_dir=CACHE_DIRS, port=7777)

