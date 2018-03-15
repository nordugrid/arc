The tools live in https://source.coderefinery.org/nordugrid/arc.git

See instructions for installation and use of python gitlab: http://python-gitlab.readthedocs.io/en/stable/index.html

You must make your private access token, see instructions: https://docs.gitlab.com/ce/user/profile/personal_access_tokens.html

The  target_id or source_id variables in the scripts refer to the project id, which you can find in Settings/General project settings. nordugrid/arc project has project id 188. 

Simplest configuration of python-gitlab, place default places /etc/.python-gitlab.cfg or  ~/.python-gitlab.cfg

```
[global]
default = coderefinery
ssl_verify = true
timeout = 5
api_version = 3

[coderefinery]
url = https://source.coderefinery.org
private_token = <your-private-token>
```

