#! /usr/bin/env python
from git import Repo
import gitlab
import os, sys, re
import argparse

"""
NB! Must be run from inside the repo you will create merge request from, and on the source branch. 

Assumes you have a configuration file in default place either /etc/.python-gitlab.cfg or ~/.python-gitlab.cfg with contents:

[global]
default = coderefinery
ssl_verify = true
timeout = 5
api_version = 3

[coderefinery]
url = https://source.coderefinery.org
private_token = xxxxxxxx

"""


target_project_id = '188'
gl = gitlab.Gitlab.from_config('coderefinery')


""" ************************************** """
""" Values you must change                 """
""" you find your fork's project id under your forks Settings/General project settings in the web interface"""

localrepo = Repo('~/arc-maikenp-fork')
source_project_id = '183'


""" End values you must change             """
""" ************************************** """

source_branch = (localrepo.active_branch).name
projects = gl.projects.list()
arc = gl.projects.get(target_project_id)
arc_issues = arc.issues.list(state='opened')
mrs = arc.mergerequests.list()
fork = gl.projects.get(source_project_id)


title = None
components = None
short_descr = None
long_descr = None
descr = None
issues_tmp = None
issues = None
issues_fix = None
issues_close = None
target_branches = None
milestone = None





""" 
#Example of filling attributes here instead of on command-line

title= 'New submission plugin for restrictive sites, to be used with a local aCT. '
components = 'client, server'
short_descr = 'LOCAL submission plugin for sites fetching jobs instead of receiving jobs.'
long_descr = 'A much longer description that can go over several lines \n. And several paragraphs.'
issues = '21,22'
issues_fix = '20'

#Target branches must match existing labels and branches in GitLab. 
target_branches = 'master,minor-6'

#Milestone must match existing milestones in GitLab, typically a release version. 
milestone = '6.0.0'

#End example.
"""




print "\n---> You can enter values of the merge request directly in this file, or on the command-line.\nIf both, the command-line arguments will overwrite the file variables.\n"

""" If values are not given in file, require as arguments"""
parser = argparse.ArgumentParser()
parser.add_argument('-si', '--source_id', help='Source project id (your forks project_id)',required = (None==source_project_id) )
parser.add_argument('-t', '--title', help='Descriptive title',required = (None==title) )
parser.add_argument('-c', '--components', help='Which component(s) does it belong to. Comma-separated list: 1:Core, 2:Server 3:Infosys, 4:Accounting, 5:Client',choices=['1','2','3','4','5'],required = (None==components) )
parser.add_argument('-sd', '--short_descr', help='Short one-line description of the work',required = (None==short_descr) )
parser.add_argument('-ld','--long_descr', help='Detailed description of the work',required = (None==long_descr) )
parser.add_argument('-if','--issues_fix',help='The fixes issue-id(s), comma-separated (no spaces)',required=False)
parser.add_argument('-ic','--issues_close',help='The closes issue-id(s), comma-separated',required=False)
parser.add_argument('-i','--issues',help='The issue-id(s), comma-separated')
parser.add_argument('-tb', '--target_branches', help='Comma separated list of target branches matching existing labels (no spaces!) e.g. minor-6,master',required = (None==target_branches))
parser.add_argument('-m','--milestone', help='Enter tag-number',required=False)
parser.add_argument('-b', '--bugzilla_link', help='Link to bugzilla ticket if there is one')


args = parser.parse_args()
if args.title: title = args.title
if args.components: components = args.components
if args.short_descr: short_descr = args.short_descr
if args.long_descr: long_descr = args.long_descr
if args.issues: issues_tmp = args.issues
if args.issues_fix: issues_fix = args.issues_fix
if args.issues_close: issues_close = args.issues_close
if args.target_branches: target_branches = args.target_branches
if args.bugzilla_link: bugz_link = args.bugzilla_link
if args.milestone: milestone = args.milestone

components = components.split(',')
if issues_tmp: issues_tmp = issues_tmp.split(',')
if issues_fix: issues_fix = issues_fix.split(',')
if issues_close: issues_close = issues_close.split(',')
issues = []

""" Filter out any duplicate issues """
if issues_tmp:
    for issue in issues_tmp:
        if issues_fix:
            if issue in issues_fix:
                continue
        if issues_close:
            if issue in issues_close:
                continue
    
        issues.append(issue)

        
target_branches = target_branches.split(',')


if not (issues or issues_fix or issues_close):
    print 'You must specify an issue either as -i (issue) as -if (fix issue) or -ic (close issue) - please run again'
    sys.exit()

def yes_no(question):
    while True:
        reply = str(raw_input(question+' (y/n): ')).lower().strip()
        if reply[:1] == 'y':
            return True
        if reply[:1] == 'n':
            return False



def check_attributes(mr):

    all_issues = []
    """ Check if issues actually exist and are not closed """
    if mr['issues']: all_issues.append(mr['issues'])
    if mr['issues_fix']: all_issues.append(mr['issues_fix'])
    if mr['issues_close']:all_issues.append(['issues_close'])

    for issue in all_issues:
        try:
            arc.issues.list(iid=issue)[0]
        except:
            print 'Error: Issues #' + issue + ' not found in target project  ', arc.name_with_namespace
            return False
    
        
    """ Check if target branch exists """
    for branch in target_branches:
        try:
            arc.branches.get(branch)
        except:
            print 'Error: Branch ' + branch + ' does not exists in project ', arc.name_with_namespace 
            return False

        
    """ Check if source branches exist """
    try:
        fork.branches.get(source_branch)
    except:
        print 'Error: Branch ' + source_branch + ' does not exists in project ', fork.name_with_namespace
        return False
    
    return True

def construct_descr():
    global descr, short_descr, long_descr
    descr = short_descr + '\n\n' + long_descr

    
def construct_title():
    global title
    issue_str = construct_issues()
    title += ' ' + issue_str

def construct_issues():

    global issues, issues_fix, issues_close

    issue_str = ''
    if issues:
        for issue in issues:
            issue_str += '#'+issue+', '
    issue_str = issue_str[:-2]

    issue_close_str = ''
    if issues_close:
        for issue in issues_close:
            issue_close_str += 'Closes #'+issue+', '
    issue_close_str = issue_close_str[:-2]

    issue_fix_str = ''
    if issues_fix:
        for issue in issues_fix:
            issue_fix_str += 'Fixes #'+issue+', '
    issue_fix_str = issue_fix_str[:-2]

    full_issue = '('
    if issue_str:
        full_issue += issue_str
    if issue_close_str:
        full_issue += ', ' + issue_close_str
    if issue_fix_str:
        full_issue += ', ' + issue_fix_str
    full_issue += ')'
    
    return full_issue.replace('(, ','(')



    
""" Construct merge requests """

construct_title()
construct_descr()
for branch in target_branches:

    merge_request = {
        'source_branch':source_branch,
        'source_project_id':source_project_id,
        'target_branch':branch,
        'target_project_id':target_project_id,
        'issues': issues,
        'issues_fix': issues_fix,
        'issues_close': issues_close,
        'title':title,
        'descr':descr
        }

    is_good = check_attributes(merge_request)
    if not is_good:
        print "Merge request not correctly constructed, exiting..."
        sys.exit()


    print "\n----> Creating merge request with attributes:"
    for key, val in merge_request.iteritems():
        print key, ':', val
    proceed = yes_no("Send merge request?")


    if not proceed:
        print 'User aborted merge request, exiting ... '
        sys.exit()
        

    print "Sending merge request"
    fork.mergerequests.create({'source_branch': source_branch,
                               'source_project_id': source_project_id,
                               'target_branch': branch,
                               'target_project_id': target_project_id,
                               'title': title,
                               'description':short_descr + '\n\n' + long_descr,
                               'milestone':milestone,
                               'labels': ['python-gitlab']})


