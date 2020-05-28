from __future__ import print_function
from __future__ import absolute_import

from .ControlCommon import *
try:
   from .Jobs import JobsControl
except ImportError:
   JobsControl = None
import subprocess
import sys
import re
import glob
import pickle
import time
import pwd
import signal

try:
   input = raw_input  # Redefine for Python 2
except NameError:
   pass

class DataDeliveryControl(ComponentControl):
    def __init__(self, arcconfig):
        self.logger = logging.getLogger('ARCCTL.DataDelivery')
        self.control_dir = None
        self.arcconfig = arcconfig
        self.control_dir = None

        # arcctl is inside arex package as well as gm-jobs
        self.gm_jobs = ARC_LIBEXEC_DIR + '/gm-jobs'
        if not os.path.exists(self.gm_jobs):
            self.logger.error('A-REX gm-jobs is not found at %s. It seams you A-REX install is broken.', self.gm_jobs)
            sys.exit(1)
        # config is mandatory
        if arcconfig is None:
            self.logger.error('Failed to get parsed arc.conf. DataDelivery control is not possible.')
            sys.exit(1)

        self.cache_min_jobs = 1000
        self.cache_ttl = 30
        self.jobs = {}
        self.process_job_log_file = False   # dummy assignment for job_log follow


        # controldir is mandatory
        self.control_dir = self.arcconfig.get_value('controldir', 'arex').rstrip('/')
        if self.control_dir is None:
            self.logger.critical('Jobs control is not possible without controldir.')
            sys.exit(1)

    def __calc_timewindow(self,args):
       twindow = None
       if args.days:
          twindow =  datetime.timedelta(days=args.days)
       if args.hours:
          if twindow:
             twindow = twindow + datetime.timedelta(hours=args.hours)
          else:
             twindow = datetime.timedelta(hours=args.hours)
       if args.minutes:
          if twindow:
             twindow = twindow + datetime.timedelta(minutes=args.minutes)
          else:
             twindow = datetime.timedelta(minutes=args.minutes)
       if args.seconds:
          if twindow:
             twindow = twindow + datetime.timedelta(seconds=args.seconds)
          else:
             twindow = datetime.timedelta(seconds=args.seconds)

       twindow_start=datetime.datetime.now() - twindow
       return twindow_start

    
    def __get_tstamp(self,line):
        """ Extract timestamp from line of type 
        [2016-08-22 15:17:49] [INFO] ......
        """
        
        words = re.split(' +', line)
        words = [word.strip() for word in words]
        timestmp_str = words[0].strip('[') + ' ' + words[1].strip(']')
        timestamp = datetime.datetime.strptime(timestmp_str,'%Y-%m-%d %H:%M:%S')

        return timestmp_str, timestamp


    def __get_time_ds(self,err_f):

       ds_time={'start':'','end':'','dt':'','done':False,'failed':False}
       ds_start = None
       ds_end = None
       with open(err_f,'r') as f:
          for line in f:

             words = re.split(' +', line)
             words = [word.strip() for word in words]
             
             if "ACCEPTED -> PREPARING" in line:
                
                timestmp_str = words[0].strip('[')
                timestmp_str = timestmp_str.strip(']')
                ds_time['start']=timestmp_str

                timestamp = datetime.datetime.strptime(timestmp_str,'%Y-%m-%dT%H:%M:%SZ')
                ds_start = timestamp

             elif "PREPARING -> SUBMIT" in line:

                timestmp_str = words[0].strip('[')
                timestmp_str = timestmp_str.strip(']')
                ds_time['end']=timestmp_str

                timestamp = datetime.datetime.strptime(timestmp_str,'%Y-%m-%dT%H:%M:%SZ')
                ds_end = timestamp

                ds_time['done']=True
                continue

             elif "PREPARING -> FINISHING" in line and 'failure' in line:
                ds_time['failed']=True
                continue
          
             else:
                continue
                

       if ds_start and ds_end:
          ds_time['dt']=str(ds_end - ds_start)
       elif ds_start:
          ds_time['dt']=str(datetime.datetime.now() - ds_start)
       else:
          return None

       return ds_time

    def get_job_time_datastaging(self,args):

       datastaging_time={}
       err_f = self.control_dir + '/job.'+args.jobid +'.errors'
       datastaging_time=self.__get_time_ds(err_f)
       print('Datastaging duration for jobid {0:<50}'.format(args.jobid))
       if datastaging_time['done']:
          print('\t{0:<21}\t{1:<21}\t{2:<12}'.format('Start','End','Duration'))
          print("\t{start:<21}\t{end:<21}\t{dt:<12}".format(**datastaging_time))
       else:
          print('\tDatastaging still ongoing')
          print('\t{0:<21}\t{1:<12}'.format('Start','Duration'))
          print("\t{0:<21}\t{1:<12}".format(datastaging_time['start'],datastaging_time['dt']))

       
    def get_summary_times_datastaging(self,args):
       """ Overview over duration of all datastaging processes in the chosen timewindow """
       datastaging_times={}

       twindow_start = self.__calc_timewindow(args)

       err_all = glob.glob(self.control_dir + "/job*.errors")
       for err_f in err_all:
          mtime=datetime.datetime.fromtimestamp(os.path.getmtime(err_f))

          """ Skip all files that are modified before the users chosen timewindow """
          if mtime < twindow_start:
             continue

          jobid = err_f.split('.')[-2]
          if self.__get_time_ds(err_f):
             datastaging_times[jobid]=self.__get_time_ds(err_f)


       done_dict={}
       ongoing_dict={}
       failed_dict={}
       for key,val in datastaging_times.iteritems():
          if not val['failed']:
             if val['done']:
                done_dict[key]=val
             else:
                ongoing_dict[key]=val
          else:
             failed_dict[key]=val

       sorted_dict = sorted(done_dict.items(), key = lambda x: x[1]['dt'])
       print('Sorted list of jobs where datastaging is done:')
       for item in sorted_dict:
          print('\t{0}: {1}'.format(item[0],item[1]['dt']))


       sorted_dict = sorted(ongoing_dict.items(), key = lambda x: x[1]['dt']) 
       print('Sorted list of jobs where datastaging is ongoing:')
       for item in sorted_dict:
          print('\t{0}: {1}'.format(item[0],item[1]['dt']))


       sorted_dict = sorted(failed_dict.items(), key = lambda x: x[1]['dt']) 
       print('Datastaging used for failed jobs:')
       for item in sorted_dict:
          print('\t{0}: {1}'.format(item[0],item[1]['dt']))



    def __parse_dtrstate(self):
        dtrstate_path = self.control_dir + '/dtr.state'
        self.dtrlog_content = open(dtrstate_path,'r').readlines()

    def __parse_errorsfile(self,args):
       """  
       1. Open job.args.jobid.erros file
       2. Parse through lines and filter for DTR 
       3. Construct a dict with key=DTRid, filename=,state=,started=,ended=,chunks_s=[s1,s2,s1],chunks_b=[b1,b2,b3],chunks_bs=[bs1,bs2,bs3]
       4. 
       """
    def __sort_count_print(self,this_list,name):
        """ Counts and prints out results """

        count_dict = {}
        this_key = ''
        if this_list:
           print('{0: >20} {1: >5}'.format('Result','count'))
           for elem in set(this_list):
              count = this_list.count(elem)
              if name not in count_dict.keys():
                 count_dict[name] = {}
              count_dict[name][elem]=count

              print('{0: >20} {1:>5}'.format(elem,str(count)))
        else:
           print('No ' + name + ' found')



    def get_current_download_endpoints(self,args):
        self.__parse_dtrstate()
        
        endpoints=[]
        cmd = "lsof -a -itcp  |  grep -v gsiftp | grep arc-dmcgr | awk '{print $9}'"

        try:
           p = subprocess.Popen(cmd,stdout=subprocess.PIPE, stderr=subprocess.STDOUT,shell=True)
           res = p.communicate()[0]
        except:
           return
        
        for r in res.splitlines():
           r = r.split('->')[-1]
           r = r.split(':')[0]
           endpoints.append(r)

        self.__sort_count_print(endpoints,'Ongoing download endpoints')
                     

    def control(self, args):


        if args.action == 'inputfiles':
           self.get_job_inputfiles(args)

        if args.action == 'summary_time_ds':
           self.get_summary_times_datastaging(args)


        if args.action == 'job_time_ds':
           self.get_job_time_datastaging(args)


           
    @staticmethod
    def register_parser(root_parser):

       __JOB_STATES = ['ACCEPTED', 'PREPARING', 'SUBMIT', 'INLRMS', 'FINISHING', 'FINISHED', 'DELETED', 'CANCELING']
       dds_ctl = root_parser.add_parser('dds',help='DataDelivery info')
       dds_ctl.set_defaults(handler_class=DataDeliveryControl)
       dds_actions = dds_ctl.add_subparsers(title='DataDelivery Control Actions',dest='action',
                                            metavar='ACTIN', help='DESCRIPTION')
       dds_actions.requireds = True
        
       
       dds_summary_time_ds = dds_actions.add_parser('summary_time_ds',help='Overview of datastaging durations (time that jobs in preparing state) within selected timewindow')
       dds_summary_time_ds.add_argument('-d','--days',default=0,type=int,help='Duration in days (default: %(default)s days)')
       dds_summary_time_ds.add_argument('-hr','--hours',default=1,type=int,help='Duration in hours (default: %(default)s hour)')
       dds_summary_time_ds.add_argument('-m','--minutes',default=0,type=int,help='Duration in minutes (default: %(default)s minutes)')
       dds_summary_time_ds.add_argument('-s','--seconds',default=0,type=int,help='Duration in seconds (default: %(default)s seconds)')


       dds_job_time_ds = dds_actions.add_parser('job_time_ds',help='Datastaging duration for a certain job (time that the job spent in reparing state)')
       dds_job_time_ds.add_argument('jobid',help='Job ID')





