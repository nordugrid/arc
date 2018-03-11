#! /usr/bin/env python
import gitlab

### Very basic script showing how to create multiple merge request with same content to several branches
### fork-id must be changed along with other obvious variables

gl = gitlab.Gitlab.from_config('coderefinery')
arc_id = '164'
arc = gl.projects.get(arc_id)

#fork_id = '175'
#my_fork = gl.projects.get(fork_id)
#source_branch = 'maikenp-dev-branch-for-issue1'
#title = 'Infosys: Some short description (Fixes #1)'
#'Short description. \n Long description over several lines and so on..... xxxxxxx'


##Example how to create merge-request. 

#mr = my_fork.mergerequests.create({'source_branch': source_branch,
#                               'source_project_id':fork_id,
#                               'target_branch': 'master',
#                               'target_project_id':arc_id,
#                               'title': title,
#                               'description':descr,
#                               'labels': ['master', 'minor-6']})

#mr = my_fork.mergerequests.create({'source_branch': source_branch,
#                               'source_project_id':fork_id,
#                               'target_branch': 'minor-6',
#                               'target_project_id':arc_id,
#                               'title': title,
#                               'description':'descr,
#                               'labels': ['master', 'minor-6']})






