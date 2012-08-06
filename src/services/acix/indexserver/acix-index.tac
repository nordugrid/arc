from acix.indexserver import indexsetup


URLS = [ 'https://localhost:5443/data/cache' ]


application = indexsetup.createIndexApplication(urls=URLS)


