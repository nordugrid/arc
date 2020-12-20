from .ControlCommon import *
import subprocess
import sys
import re
import glob

try:
    from .Jobs import *
except ImportError:
    JobsControl = None

def complete_job_id(prefix, parsed_args, **kwargs):
    arcconf = get_parsed_arcconf(parsed_args.config)
    return JobsControl(arcconf).complete_job(parsed_args)



class DataStagingControl(ComponentControl):
    def __init__(self, arcconfig):
        self.logger = logging.getLogger('ARCCTL.DataStaging')
        self.control_dir = None
        self.arcconfig = arcconfig

        # config is mandatory
        if arcconfig is None:
            self.logger.error('Failed to get parsed arc.conf. DataStaging control is not possible.')
            sys.exit(1)
            
        # controldir is mandatory
        self.control_dir = self.arcconfig.get_value('controldir', 'arex').rstrip('/')
        if self.control_dir is None:
            self.logger.critical('Jobs control is not possible without controldir.')
            sys.exit(1)

    def _get_tstamp(self,line):
        """ Extract timestamp from line of type 
        [2016-08-22 15:17:49] [INFO] ......
        """
        
        words = re.split(' +', line)
        words = [word.strip() for word in words]
        timestmp_str = words[0].strip('[') + ' ' + words[1].strip(']')
        timestamp = datetime.datetime.strptime(timestmp_str,'%Y-%m-%d %H:%M:%S')

        return timestmp_str, timestamp


    def _calc_timewindow(self,args):
        twindow = None
        if args.days:
            twindow =  datetime.timedelta(days=args.days)
        if args.hours:
            """ This has a default of 1 hr """
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

    
    def _get_tstamp(self,line):
        """ Extract timestamp from line of type 
        [2016-08-22 15:17:49] [INFO] ......
        """
        
        words = re.split(' +', line)
        words = [word.strip() for word in words]
        timestmp_str = words[0].strip('[') + ' ' + words[1].strip(']')
        timestamp = datetime.datetime.strptime(timestmp_str,'%Y-%m-%d %H:%M:%S')
        
        return timestmp_str, timestamp


    def _get_filecount_joblog(self,log_f,jobid,twindow_start=None):
        pass
        
    def _get_timestamps_joblog(self,log_f,jobid,twindow_start=None):

        """ 
        Get the total time from PREPARING to SUBMIT which is the total time in PREPARING.
        Only calculate the time for the jobs that actually have user-deinfed input-files (not pilot-related).
        Last filter here, to ensure that only jobs that actually were in preparing in the timewindow are counted, in the case where a timewindow is used. 
        """

        ds_time={'start':'','end':'','dt':'','done':False,'failed':False,'noinput':False}
        has_udef_input = self._has_userdefined_inputfiles(jobid)

        if has_udef_input is None:
            print('The grami file  ', grami_file, ' is no longer present. Skipping this job.')
        elif has_udef_input is False:
            ds_time={'start':'','end':'','dt':'','done':False,'failed':False,'noinput':True}
        else:
            """ This job has user-defined inputfiles for datastaging """
            ds_start = None
            ds_end = None
            with open(log_f,'r') as f:
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
                        """ if this line does not contain target string, continue to next line """
                        continue
                        
            if twindow_start and ds_end:
                """  If the datastaging started """
                if ds_end < twindow_start:
                    return None

            if ds_start and ds_end:
                """ Datastaging done """
                ds_time['dt']=str(ds_end - ds_start)
            elif ds_start:
                """  Datastaging ongoing """
                ds_time['dt']=str(datetime.datetime.utcnow() - ds_start)
            else:
                return None
        return ds_time

    def _has_userdefined_inputfiles(self,jobid):
        grami_file = self.control_dir + '/' + 'job.' + jobid + '.grami'
        try:
            with open(grami_file,'r') as f:
                for line in f:
                    if 'joboption_inputfile' in line:
                        fileN = line.split('=')[-1].strip()
                        fileN = fileN.replace("'/","")
                        fileN = fileN.replace("'","")
                        if not ('panda' in fileN  or 'pilot' in fileN or 'json' in fileN):
                            return True

        except OSError:
            return None
        return False


    def _get_inputfilenames(self,jobid):
        """ The grami-info only contains filename, not URI"""
        grami_file = self.control_dir + '/' + 'job.' + jobid + '.grami'
        all_files = []
        all_files_user = []
        try:
            with open(grami_file,'r') as f:
                for line in f:
                    if 'joboption_inputfile' in line:
                        fileN = line.split('=')[-1].strip()
                        fileN = fileN.replace("'/","")
                        fileN = fileN.replace("'","")
                        
                        """ ATLAS specific - these files are uploaded by client and not handled by  arex """
                        if '.json' in fileN or 'pandaJobData.out' in fileN or 'runpilot2-wrapper.sh' in fileN:
                            continue

                        if len(fileN)>0:
                            all_files.append(fileN)
                            """ Ignore some default files that are not relevant for data staging """
                            if not('panda' in fileN or 'pilot' in fileN or 'json' in fileN):
                                all_files_user.append(fileN)
        except OSError:
            """ Will just then return empty all_files and all_files_user lists """
            pass
                            
        return all_files, all_files_user

    def _get_filesforjob(self,args,jobid):

        """  Extracts information about the files already downloaded for a single job from its jobs statistics file """
        job_files_done = {}

        """ Get all files to download for job - uses grami file """
        all_files, all_files_user = self._get_inputfilenames(jobid)

        """ Get files already downloaded """
        stat_file = self.control_dir + '/' + 'job.' + jobid + '.statistics'
        with open(stat_file,'r') as f:
            for line in f:
                line = line.strip()

                if line and 'inputfile' in line:
                    words = re.split(',', line)
                    words = [word.strip() for word in words]

                    fileN = words[0].split('inputfile:')[-1].split('/')[-1]

                    """ ATLAS specific - these files are uploaded by client and not handled by  arex """
                    if 'json' in fileN or 'pandaJobData.out' in fileN or 'runpilot2-wrapper.sh' in fileN:
                        continue

                    source = (words[0].split('inputfile:')[-1]).split('=')[-1][:-len(fileN)-1]
                    size = int(words[1].split('=')[-1])/(1024*1024.)
                    start = words[2].split('=')[-1]
                    end = words[3].split('=')[-1]
                    cached = words[4].split('=')[-1]
                    atlasfile = False

                    if 'panda' in fileN or 'pilot2' in fileN:
                        atlasfile = True

                    start_dtstr = datetime.datetime.strptime(start, '%Y-%m-%dT%H:%M:%SZ')
                    end_dtstr = datetime.datetime.strptime(end, '%Y-%m-%dT%H:%M:%SZ')
                    seconds = (end_dtstr - start_dtstr).seconds

                    job_files_done[fileN] = {'size':size,'source':source,'start':start,'end':end,'seconds':seconds,'cached':cached,'atlasfile':atlasfile}

        return all_files, all_files_user, job_files_done


    def _get_dtrstates(self,args):

        
        dtrlog = self.arcconfig.get_value('statefile', 'arex/data-staging')

        if dtrlog is None:
            self.logger.critical('Job state file name can not be found in arc.conf.')
            return

        dtrlog = dtrlog.rstrip('/')
        state_counter = {}
        num_lines = 0

        if os.path.exists(dtrlog):
            with open(dtrlog,'r') as f:

                for line in f:
                    l = line.split()
                    state = l[1]
                    if state == 'TRANSFERRING':
                        if len(l) == 6:
                            host = l[5]
                            state = state + '_' + host
                        else:
                            state = state + '_local'
                    if state in state_counter:
                        state_counter[state] = state_counter[state] + 1
                    else:
                        state_counter[state] = 1
                    num_lines = num_lines + 1

                state_counter['ARC_STAGING_TOTAL']=num_lines
        
        else:
            self.logger.error('Failed to open DTR state file: %s',dtrlog)

        return  state_counter


    def show_dtrstates(self,args):

        state_counter = self._get_dtrstates(args)

        """ TO-DO print in nice order """
        print_order = ['CACHE_WAIT','STAGING_PREPARING_WAIT','STAGE_PREPARE','TRANSFER_WAIT','TRANSFER','PROCESSING_CACHE']
        if state_counter:
            print('Number of current datastaging processes (files):')
            print('\t{0:<25}{1:<20}{2:<6}'.format('State','Data-delivery host', 'Number'))
            
            count_transferring = 0

            """ First print the most important states:"""
            for state in print_order:
                try:
                    print('\t{0:<25}{1:<20}{2:>6}'.format(state,'N/A',state_counter[state]))
                except KeyError:
                    pass

            """ Now print the TRANSFERRING states"""
            for key, val in state_counter.items():
                if 'TRANSFERRING' in key:
                    state = 'TRANSFERRING'
                    host = key.split('_')[-1]
                    count_transferring += val
                    if 'local' in host:
                        continue
                    print('\t{0:<25}{1:<20}{2:>6}'.format(state,host,val))

            """  Print out other states """
            for key, val in state_counter.iteritems():
                if 'TRANSFERRING' in key or key in print_order or 'local' in key or key in 'ARC_STAGING_TOTAL': continue
                print('\t{0:<25}{1:<20}{2:>6}'.format(key,'N/A',val))
                    

            try:
                count_transferring += state_counter['TRANSFERRING_local']
                print('\t{0:<25}{1:<20}{2:>6}'.format('TRANSFERRING','local',state_counter['TRANSFERRING_local']))
            except KeyError:
                pass

            """ Print out divider """
            print('-'*60)
            """ Sum up all TRANSFERRING slots """
            print('\t{0:<25}{1:<20}{2:>6}'.format('TRANSFERRING TOTAL','N/A',count_transferring))

            """ Finally print the sum of all dtrs """
            print('\t{0:<25}{1:<20}{2:>6}'.format('ARC_STAGING_TOTAL','N/A',state_counter['ARC_STAGING_TOTAL']))

                
        else:
            print('No information in %s file: currently no datastaging processes running',dtrlog)
                
        return


    def show_job_time(self,args):

        datastaging_time={}
        jobid = args.jobid
        log_f = self.control_dir + '/job.'+jobid +'.errors'
        datastaging_time=self._get_timestamps_joblog(log_f,jobid)

        if datastaging_time:
            print('\nDatastaging durations for jobid {0:<50}'.format(args.jobid))
            if datastaging_time['noinput']:
                print('\tThis job has no user-defined input-files, hence no datastaging needed/done.')
            else:
                if datastaging_time['done']:
                    print('\t{0:<21}\t{1:<21}\t{2:<12}'.format('Start','End','Duration'))
                    print("\t{start:<21}\t{end:<21}\t{dt:<12}".format(**datastaging_time))
                else:
                    print('\tDatastaging still ongoing')
                    print('\t{0:<21}\t{1:<12}'.format('Start','Duration'))
                    print("\t{0:<21}\t{1:<12}".format(datastaging_time['start'],datastaging_time['dt']))
        else:
            print('No datastaging information for jobid {0:<50} - Try arcctl accounting instead - the job might be finished.'.format(args.jobid))



    def show_job_details(self,args):
        

        all_files, all_files_user, job_files_done = self._get_filesforjob(args,args.jobid)
    
        """ Collect remaining files to be downloaded """
        tobe_stagedin = []
        for fileN in all_files:
            if fileN not in job_files_done.keys():
                tobe_stagedin.append(fileN)

        """ General info """
        print('\nInformation  about input-files for jobid {} '.format(args.jobid))
        
        """  Print out a list of all files and if staged-in or not """
        print('\nState of input-files:')
        print('\t{0:<8}{1:<60}{2:<12}'.format('COUNTER','FILENAME','STAGED-IN'))
        for idx,fileN in enumerate(all_files):
            staged_in = 'no'
            if fileN in job_files_done.keys():
                staged_in = 'yes'
            print('\t{0:<8}{1:<60}{2:<12}'.format(idx+1,fileN,staged_in))
        print('\tNote: files uploaded by the client appear to not be staged-in, ignore these as AREX does not handle the stage-in of these files.')
                
        """ Print out information about files already downloaded """
        sorted_dict = sorted(job_files_done.items(), key = lambda x: x[1]['end'])
        print('\nDetails for files that have been staged in - both downloaded and cached:')
        print('\t{0:<8}{1:<60}{2:<60}{3:<15}{4:<25}{5:<25}{6:<10}{7:<7}'.format('COUNTER','FILENAME','SOURCE','SIZE (MB)','START','END','SECONDS','CACHED'))
        for idx,item in enumerate(sorted_dict):
            print("\t{0:<8}{1:<60}{2:<60}{3:<15.3f}{4:<25}{5:<25}{6:<10}{7:<7}".format(idx+1,item[0],item[1]['source'],item[1]['size'],item[1]['start'],item[1]['end'],item[1]['seconds'],item[1]['cached']))

        return job_files_done
        

    def show_job_finegrained(self,args,job_files_done):

        dtr_file = {}
        file_dtr = {}

        log_file = self.control_dir + '/' + 'job.' + args.jobid + '.errors'
        with open(log_file,'r') as f:
            for line in f:

                words = re.split(' +', line)
                words = [word.strip() for word in words]


                if 'Scheduler received new DTR' in line:
                    
                    """ First instance of the new DTR """
                    timestmp_str, timestmp_obj = self._get_tstamp(line)
                    dtrid_short = words[5].replace(':','')
                    fileN = words[13].split('/')[-1]
                    fileN = fileN.replace(",","")
                    
                    dtr_file[dtrid_short]=fileN
                    file_dtr[fileN]=dtrid_short

                    if fileN in job_files_done.keys():
                        job_files_done[fileN]['dtrid_short']=dtrid_short
                        job_files_done[fileN]['start_sched']=timestmp_str
                    else:
                        job_files_done[fileN]={}
                        job_files_done[fileN]['dtrid_short']=dtrid_short
                        job_files_done[fileN]['start_sched']=timestmp_str

                elif 'Delivery received new DTR' in line:

                    #try:
                    """ Delivery actually starting - waiting time over """
                    timestmp_str, timestmp_obj = self._get_tstamp(line)
                    dtrid_short = words[5].replace(':','')
                    fileN = dtr_file[dtrid_short]
                    
                    job_files_done[fileN]['start_deliver']=timestmp_str

                elif 'Transfer finished' in line:

                    """ Transfer done, file downloaded  """
                    timestmp_str, timestmp_obj = self._get_tstamp(line)
                    dtrid_short = words[5].replace(':','')
                    fileN = dtr_file[dtrid_short]
                    
                    job_files_done[fileN]['transf_done']=timestmp_str


                elif 'Returning to generator' in line:

                    """ DTR done """ 
                    timestmp_str, timestmp_obj = self._get_tstamp(line)
                    dtrid_short = words[5].replace(':','')
                    fileN = dtr_file[dtrid_short]
                    
                    job_files_done[fileN]['return_gen']=timestmp_str


        """ Calculated avg download speed """
        """ Calculated by diff in time between 'Delivery received new DTR' and 'Transfer finished 
        messages for the DTR. '"""
        for key,val in job_files_done.items():
            if 'size' in val.keys() and 'start_deliver' in val.keys():
                start_dwnld = datetime.datetime.strptime(val['start_deliver'], "%Y-%m-%d %H:%M:%S")
                end_dwnld = datetime.datetime.strptime(val['transf_done'], "%Y-%m-%d %H:%M:%S")
                seconds = (end_dwnld - start_dwnld).seconds
                speed = float(val['size']/seconds)
                job_files_done[key]['speed']=speed
                job_files_done[key]['seconds']=seconds
                

        """ Print out information about files already downloaded """
        """ TO-DO find a nice way to sort this, maybe removing the files that do not have all info provided? """
        downloads = False
        print('\nFine-grained details for files that have been staged-in by download (not cached):')
        print('\t{0:<8}{1:<60}{2:<15}{3:<22}{4:<22}{5:<22}{6:<22}{7:<22}{8:<22}{9:<20}{10:<20}'.format('COUNTER','FILENAME','SIZE (MB)','START','END','SCHEDULER-START','DELIVERY-START','TRANSFER-DONE','ALL-DONE','DWLD-DURATION (s)','CALC AVG DWLD-SPEED MB/s'))
        idx = 1
        for key,val in job_files_done.items():
            if ('start_deliver' in val.keys() and 'start_sched' in val.keys() and 'return_gen' in val.keys() and 'speed' in val.keys()):
                print("\t{0:<8}{1:<60}{2:<15.3f}{3:<22}{4:<22}{5:<22}{6:<22}{7:<22}{8:<22}{9:<20}{10:<20.3f}".format(idx,key,val['size'],val['start'],val['end'],val['start_sched'],val['start_deliver'],val['transf_done'],val['return_gen'],val['seconds'],val['speed']))
                idx += 1
                downloads = True
        if not downloads:
            print('\tNo download info available, probably because all files for this jobs were already in the cache.')



    def show_summary_files(self,args):
        """ 
        Total number and size of files downloaded in the chosen timewindow 
        Uses the job.<jobid>.statistics file to extract files downloaded, as this gets updated file by file once a download is done.
        """

        n_jobs = 0 

        n_files_all = 0
        n_files_all_user = 0
        n_files =  0
        n_files_atlas =  0
        n_files_user =  0
        n_files_cached = 0
        n_files_cached_atlas = 0
        n_files_cached_user = 0
        n_files_downloaded = 0
        n_files_downloaded_atlas = 0
        n_files_downloaded_user = 0

        size_files = 0
        size_files_atlas = 0
        size_files_user = 0
        size_files_cached = 0
        size_files_cached_atlas = 0
        size_files_cached_user = 0
        size_files_downloaded = 0
        size_files_downloaded_atlas = 0
        size_files_downloaded_user = 0

        datastaging_files={}
        twindow_start = self._calc_timewindow(args)

        print('\nThis may take some time... Fetching the total number of files downloaded for jobs modified after {}'.format(datetime.datetime.strftime(twindow_start,'%Y-%m-%d %H:%M:%S\n')))

        log_all = glob.glob(self.control_dir + "/job.*.statistics")
        for log_f in log_all:
            mtime = None
            try:
                mtime=datetime.datetime.fromtimestamp(os.path.getmtime(log_f))
            except OSError:
                """  Files got removed in the meantime, skip this job """
                continue
        
            """ Skip all jobs that are last modified before the users or default timewindow """
            if mtime < twindow_start:
                continue

            jobid = log_f.split('.')[-2]
            all_files, all_files_user, job_files_done = self._get_filesforjob(args,jobid)

            n_jobs += 1
            n_files_all += len(all_files)
            n_files_all_user += len(all_files_user)


            """ Collecting information for files that are done in stage-in for this job """
            for key, val in job_files_done.iteritems():
                """  Only files in active download during the timewindow are added """
                end = datetime.datetime.strptime(val['end'], '%Y-%m-%dT%H:%M:%SZ')
                if end < twindow_start:
                    continue
                
                n_files += 1
                """  Originally converted to MB, want GB here """
                size_files += val['size']/1024.

                if val['atlasfile']:
                    n_files_atlas += 1
                    size_files_atlas += val['size']/1024.
                else:
                    n_files_user += 1
                    size_files_user += val['size']/1024.

                    
                if val['cached'] == 'yes':
                    n_files_cached += 1
                    size_files_cached += val['size']/1024.
                    if val['atlasfile']:
                        n_files_cached_atlas += 1
                        size_files_cached_atlas += val['size']/1024.
                    else:
                        n_files_cached_user += 1
                        size_files_cached_user += val['size']/1024.
                else:
                    n_files_downloaded += 1
                    size_files_downloaded += val['size']/1024.
                    if val['atlasfile']:
                        n_files_downloaded_atlas += 1
                        size_files_downloaded_atlas += val['size']/1024.
                    else:
                        n_files_downloaded_user += 1
                        size_files_downloaded_user += val['size']/1024.


        if not n_jobs:
            print('\nNo active jobs were found in the selected timewindow --> exiting.')
            return 

        
        print('\nTimewindow start: : '+ datetime.datetime.strftime(twindow_start,'%Y-%m-%d %H:%M:%S'))
        print('\nALL JOBS')
        print('Number of jobs active since start of timewindow '+ str(n_jobs))
        print('\tTotal number of inputfiles for these jobs: ' + str(n_files_all))
        print('\tOut of these, user-defined input-files amounted to: ' + str(n_files_all_user))
    
        if not n_files:
            print('\nNo files were done staging-in during the selected timewindow --> exiting.')
            return

        print('\nFILES DONE IN STAGE-IN')
        print('{0:<50}{1:<10}{2:<15}'.format('','NUMBER','SIZE [GB]'))
        print('{0:<50}{1:<10}{2:<15.1f}'.format('Total',str(n_files),size_files))
        

        if n_files_atlas:
            print('{0:<50}{1:<10}{2:<15.1f}'.format('    User-defined',str(n_files_user),size_files_user))
            print('{0:<50}{1:<10}{2:<15.1f}'.format('    Atlas-files',str(n_files_atlas),size_files_atlas))


        print('{0:<50}{1:<10}{2:<15.1f}'.format('Cached Total',str(n_files_cached),size_files_cached))
        if n_files_atlas:
            print('{0:<50}{1:<10}{2:<15.1f}'.format('    User-defined',str(n_files_cached_user),size_files_cached_user))
            print('{0:<50}{1:<10}{2:<15.1f}'.format('    Atlas-files',str(n_files_cached_atlas),size_files_cached_atlas))

        
        print('{0:<50}{1:<10}{2:<15.1f}'.format('Not-cached Total (downloaded)',str(n_files_downloaded),size_files_downloaded))
        if n_files_atlas:
            print('{0:<50}{1:<10}{2:<15.1f}'.format('    User-defined',str(n_files_downloaded_user),size_files_downloaded_user))
            print('{0:<50}{1:<10}{2:<15.1f}'.format('    Atlas-files',str(n_files_downloaded_atlas),size_files_downloaded_atlas))


        print('\n\nRATIOS')
        print('{0:<50}{1:<20}{2:<20}'.format('','RATIO amount','RATIO size'))
        if n_files_atlas:
            try:
                print('{0:<50}{1:<20.4}{2:<20.4}'.format('Total User/Atlas',(float(n_files_user)/float(n_files_atlas)),float(size_files_user/size_files_atlas)))
            except ZeroDivisionError:
                pass
        try:
            print('{0:<50}{1:<20.4}{2:<20.4}'.format('Total Cached/non-cached',(float(n_files_cached)/float(n_files_downloaded)),float(size_files_cached/size_files_downloaded)))
        except ZeroDivisionError:
            pass
        if n_files_atlas:
            try:
                print('{0:<50}{1:<20.4}{2:<20.4}'.format('Total Cached-user/Cached-atlas',(float(n_files_cached_user)/float(n_files_cached_atlas)),float(size_files_cached_user/size_files_cached_atlas)))
            except ZeroDivisionError:
                pass
            try:
                print('{0:<50}{1:<20.4}{2:<20.4}'.format('Total Downloaded-user/Downloaded-atlas',(float(n_files_downloaded_user)/float(n_files_downloaded_atlas)),float(size_files_downloaded_user/size_files_downloaded_atlas)))
            except ZeroDivisionError:
                pass


    def show_summary_jobs(self,args):
        
        """ Overview over duration of all datastaging processes in the chosen timewindow 
        Checks job.<jobid>.errors files that have been modified during the timewindow. 
        Checks duration between ACCEPTED -> PREPARING to PREPARING -> FINISHING stages. 
        """
        datastaging_jobs={}
        twindow_start = self._calc_timewindow(args)

        print('\nThis may take some time... Fetching summary of download times for jobs modified after {}'.format(datetime.datetime.strftime(twindow_start,'%Y-%m-%d %H:%M:%S')))

        log_all = glob.glob(self.control_dir + "/job.*.errors")
        for log_f in log_all:
            mtime = None
            try:
                mtime=datetime.datetime.fromtimestamp(os.path.getmtime(log_f))
            except OSError:
                """  Files got removed in the meantime, skip this job """
                continue
        
            """ Skip all files that are modified before the users or default timewindow """
            if mtime < twindow_start:
                continue

            jobid = log_f.split('.')[-2]
            try:
                jobdict = self._get_timestamps_joblog(log_f,jobid,twindow_start)
                if jobdict:
                    datastaging_jobs[jobid]=jobdict
            except:
                continue


        done_dict={}
        ongoing_dict={}
        failed_dict={}
        noinput_list=[]
        for key,val in datastaging_jobs.iteritems():
            if not val['failed']:
                if val['noinput']:
                    noinput_list.append(key)
                    continue
                if val['done']:
                    done_dict[key]=val
                else:
                    ongoing_dict[key]=val
            else:
                failed_dict[key]=val
        
        print('\nJobs where no user-defined input-data was defined (no datastaging done/needed): ')
        print('Number of jobs:  '+ str(len(noinput_list)))
        for jobid in noinput_list:
            print('\t{0:}'.format(jobid))
            

        sorted_dict = sorted(done_dict.items(), key = lambda x: x[1]['dt'])
        print('\nSorted list of jobs where datastaging is done:')
        print('Number of jobs:  '+ str(len(sorted_dict)))
        if sorted_dict:
            print('\t{0:<60}{1:<10}{2:<22}'.format('JOBID','DURATION','TIMESTAMP-DONE'))
            for item in sorted_dict:
                print('\t{0:<60}{1:<10}{2:<22}'.format(item[0],item[1]['dt'],item[1]['end']))


        sorted_dict = sorted(ongoing_dict.items(), key = lambda x: x[1]['dt']) 
        if ongoing_dict:
            print('\nSorted list of jobs where datastaging is ongoing:')
            print('Number of jobs:  ' + str(len(sorted_dict)))
            print('\t{0:<60}{1:<10}'.format('JOBID','DURATION'))
            for item in sorted_dict:
                print('\t{0:<60}{1:<10}'.format(item[0],item[1]['dt'].split('.')[0]))


        sorted_dict = sorted(failed_dict.items(), key = lambda x: x[1]['dt']) 
        if failed_dict:
            print('\nDatastaging used for failed jobs:')
            print('Number of jobs:  '+ str(len(sorted_dict)))
            print('\t{0:<60}{1:<10}'.format('JOBID','DURATION'))
            for item in sorted_dict:
                print('\t{0:<60}{1:<10}'.format(item[0],item[1]['dt'].split('.')[0]))

    def summarycontrol(self,args):
        if args.summaryaction == 'jobs':
            self.show_summary_jobs(args)
        if args.summaryaction == 'files':
            self.show_summary_files(args)
        
    def jobcontrol(self,args):
        if args.jobaction == 'get-totaltime':
            self.show_job_time(args)
        elif args.jobaction == 'get-details':
            job_files_done = self.show_job_details(args)
            self.show_job_finegrained(args,job_files_done)
            

    def control(self, args):
        if args.action == 'job':
            self.jobcontrol(args)
        elif args.action == 'summary':
            self.summarycontrol(args)
        elif args.action == 'dtr':
            self.show_dtrstates(args)


    @staticmethod
    def register_job_parser(dds_job_ctl):

        dds_job_ctl.set_defaults(handler_class=DataStagingControl)
        dds_job_actions = dds_job_ctl.add_subparsers(title='Job Datastaging Menu', dest='jobaction',metavar='ACTION',help='DESCRIPTION')
        
        dds_job_total = dds_job_actions.add_parser('get-totaltime', help='Show the total time spent in the preparation stage for the selected job')
        dds_job_total.add_argument('jobid',help='Job ID').completer = complete_job_id
        
        dds_job_details = dds_job_actions.add_parser('get-details', help='Show details related to the  files downloaded for the selected job')
        dds_job_details.add_argument('jobid',help='Job ID').completer = complete_job_id

        #dds_job_details.add_argument('-d', '--details', help='Detailed info about jobs files', action='store_true')

           
    @staticmethod
    def register_parser(root_parser):

        dds_ctl = root_parser.add_parser('datastaging',help='DataStaging info')
        dds_ctl.set_defaults(handler_class=DataStagingControl)


        dds_actions = dds_ctl.add_subparsers(title='DataStaging Control Actions',dest='action',
                                             metavar='ACTION', help='DESCRIPTION')
        dds_actions.required = True
        
       
        """ Summary """
        dds_summary_ctl = dds_actions.add_parser('summary',help='Job Datastaging Summary Information for jobs preparing or running.')
        dds_summary_ctl.set_defaults(handler_class=DataStagingControl)
        dds_summary_actions = dds_summary_ctl.add_subparsers(title='Job Datastaging Summary Menu',dest='summaryaction',metavar='ACTION',help='DESCRIPTION')
        
        dds_summary_jobs = dds_summary_actions.add_parser('jobs',help='Show overview of the duration of datastaging for jobs active in the chosen (or default=1hr) timewindow')
        dds_summary_jobs.add_argument('-d','--days',default=0,type=int,help='Modification time in days (default: %(default)s days)')
        dds_summary_jobs.add_argument('-hr','--hours',default=1,type=int,help='Modification time in hours (default: %(default)s hour)')
        dds_summary_jobs.add_argument('-m','--minutes',default=0,type=int,help='Modification time in minutes (default: %(default)s minutes)')
        dds_summary_jobs.add_argument('-s','--seconds',default=0,type=int,help='Modification time in seconds (default: %(default)s seconds)')
        

        dds_summary_files = dds_summary_actions.add_parser('files',help='Show the total number file and and total file-size downloaded in the chosen (or default=1hr)timewindow')
        dds_summary_files.add_argument('-d','--days',default=0,type=int,help='Modification time in days (default: %(default)s days)')
        dds_summary_files.add_argument('-hr','--hours',default=1,type=int,help='Modification time in hours (default: %(default)s hour)')
        dds_summary_files.add_argument('-m','--minutes',default=0,type=int,help='Modification time in minutes (default: %(default)s minutes)')
        dds_summary_files.add_argument('-s','--seconds',default=0,type=int,help='Modification time in seconds (default: %(default)s seconds)')
        
       
        """ Job """
        dds_job_ctl = dds_actions.add_parser('job',help='Job Datastaging Information for a preparing or running job.')
        DataStagingControl.register_job_parser(dds_job_ctl)


        """ Data delivery processes """
        dds_dtr_ctl = dds_actions.add_parser('dtr',help='Data-delivery transfer (DTR) information')
        dds_dtr_ctl.set_defaults(handler_class=DataStagingControl)
        dds_dtr_actions = dds_dtr_ctl.add_subparsers(title='DTR info menu',dest='dtr',metavar='ACTION',help='DESCRIPTION')
        
        dds_dtr_state = dds_dtr_actions.add_parser('state', help='Show summary of DTR state info')
        dds_dtr_state.add_argument('state',action='store_true')






