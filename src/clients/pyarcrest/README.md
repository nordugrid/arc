# Dependency on ARC library python bindings

Currently, the *pyarcrest.arc* module depends on [ARC](https://www.nordugrid.org/arc/arc6/index.html) library python bindings from *python3-nordugrid-arc* package available in several Linux distributions and their package repositories. The package is also available in [Nordugrid repositories](https://www.nordugrid.org/arc/arc7/common/repos/repository.html). For package to be available in a custom virtual environment, the environment has to be created with additional flag to include system site packages:

```
python3 -m venv --system-site-packages pyarcrest-venv
. pyarcrest-venv/bin/activate
pip install git+https://github.com/jakobmerljak/pyarcrest.git@dev
```
