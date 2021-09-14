#!/usr/bin/python

#########################################################################################
# Author      : Jaekyu Lee (jq.lee17@gmail.com)
# Description : Scons top-level
#########################################################################################


import os
import sys
import ConfigParser


## Check c++0x support
def CheckCPP0x():
  def SimpleCall(context):
    cpp0x_test = '''
    #include <vector>
    #include <iostream>
  
    using namespace std;

    int main(void)
    {
      vector<int> test_vector(5);
      int sum = 0;
      for (auto itr = test_vector.begin(); itr != test_vector.end(); ++itr) {
        sum += (*itr);
      }
      cout << sum << "\N";
    }
    '''
    context.Message('Checking for c++0x conformance...')
    context.env.AppendUnique(CXXFLAGS=['-std=c++0x'])
    result = context.TryCompile(cpp0x_test, '.cpp')
    context.Result(result)
    return result
  return SimpleCall


## Check c++0x support
def pre_compile_check():
  ## Environment
  custom_vars = set(['AS', 'AR', 'CC', 'CXX', 'HOME', 'LD_LIBRARY_PATH', 'PATH', 'RANLIB'])
  custom_env = {}

  for key,val in os.environ.iteritems():
    if key in custom_vars:
      custom_env[key] = val

  conf = Configure(Environment(ENV=custom_env), custom_tests = {'CheckCPP0x' : CheckCPP0x()})

  if not conf.CheckCPP0x():
    print('Error: Your compiler does not support c++0x. Exit now...')
    sys.exit()


pre_compile_check()


## Configuration
flags = {}


## Configuration from file
Config = ConfigParser.ConfigParser()
Config.read('macsim.config')
flags['dram']  = Config.get('Library', 'dram', '0')
flags['power'] = Config.get('Library', 'power', '0')
flags['iris']  = Config.get('Library', 'iris', '0')
flags['debug'] = Config.get('Build', 'debug', '0')
flags['gprof'] = Config.get('Build', 'gprof', '0')
flags['val']   = Config.get('Build_Extra', 'val', '0')


## Configuration from commandline
flags['debug'] = ARGUMENTS.get('debug', flags['debug'])
flags['gprof'] = ARGUMENTS.get('gprof', flags['gprof'])
flags['power'] = ARGUMENTS.get('power', flags['power'])
flags['iris']  = ARGUMENTS.get('iris', flags['iris'])
flags['dram']  = ARGUMENTS.get('dram', flags['dram'])
flags['val']   = ARGUMENTS.get('val', flags['val'])


## Checkout DRAMSim2 copy
if flags['dram'] == '1':
  if not os.path.exists('src/DRAMSim2'):
    os.system('git clone git://github.com/dramninjasUMD/DRAMSim2.git src/DRAMSim2')

## Checkout MCSim copy
if flags['dram'] == '2':
  if not os.path.exists('src/MCsim'):
    os.system('git clone https://github.com/uwuser/MCsim.git src/MCsim')

## Create stat/knobs
SConscript('scripts/SConscript')


## debug build
if flags['debug'] == '1':
  SConscript('SConscript', variant_dir='.dbg_build', duplicate=0, exports='flags')
  Clean('.', '.dbg_build')
## gprof build
elif flags['gprof'] == '1':
  SConscript('SConscript', variant_dir='.gpf_build', duplicate=0, exports='flags')
  Clean('.', '.gpf_build')
## opt build
else:
  SConscript('SConscript', variant_dir='.opt_build', duplicate=0, exports='flags')
  Clean('.', '.opt_build')


