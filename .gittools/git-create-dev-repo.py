#!/usr/bin/env python
from git import Repo
import gitlab


"""
See README file for installing python-gitlab and for configuration.

Script for 
1) creating an issue with labels (type, component)
2) based on labels (type) check out parent branch
3) create development branch

NB! Assumes you are working on release 6. 
If not, please change target branches from bugfix-6 or minor-6 to bugfix-7 or minor-7. 

"""



""" Variables to edit """
gl = gitlab.Gitlab.from_config('coderefinery')
path_local_fork_repo = '~/arc-maikenp-fork'


""" Example with more than 2 issues (and thus 2 dev-branches) 
keys of the dictionary must match keys from gitlab api: https://docs.gitlab.com/ee/api/issues.html """
issue_list = [
    {
        'title':'Issue to test auto-create from script',
        'description':
        'Use Python gitlab api to create issue, and from that create dev-branch on fork\n\n'\
        'Can be several lines. To add images or other more elaborate things, update from web.',
        'labels':['type-enhancement','component-client'],
        'asignees':['Maiken Pedersen']
    },
    {
        'title':'Issue to test auto-create from script',
        'description':'Use Python gitlab api to create issue, and from that create dev-branch on fork',
        'labels':['type-bugfix','component-infosys'],
        'asignees':['Maiken Pedersen']
    }
]
""" End variables to edit """



parent_project_id = '188'
arc = gl.projects.get(parent_project_id)




""" Create issues and dev-branches based on issue contents """
for issue_dict in issue_list:

    """ Improve title of issue  """
    labels = issue_dict['labels']
    components = []
    for label in labels:
        if 'component' in label:
            affix = label.split('-')[-1]            
            components.append(affix.title())

    component_str = (', ').join(components)
    issue_dict['title'] = component_str + ': ' + issue_dict['title']


    """ Create issue via api """
    print('Creating issue with content: ', issue_dict)
    issue = arc.issues.create(issue_dict)


    """ create branches for issue """
    parent_branch = ''
    if 'type-enhancement' in issue_dict['labels'] or 'type-feature' in issue_dict['labels']:
        parent_branch = 'minor-6'
    elif 'type-bugfix' in issue_dict['labels']:
        parent_branch = 'bugfix-6'
            
    repo = Repo(path_local_fork_repo)
    repo.git.checkout(parent_branch)
    new_branch = 'dev-branch-for-issue-'+str(issue.iid)
    print('Creating branch from ', parent_branch, ' with name ', new_branch)
    repo.git.checkout('HEAD',b=new_branch)








