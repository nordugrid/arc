#! /usr/bin/env python
import storage.client
a = storage.client.AHashClient('http://localhost:60002/AHash')
print a.get(['0'])
