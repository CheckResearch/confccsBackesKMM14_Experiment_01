#! /usr/bin/env python

import sys

sys.path.append('stem/')

from stem.descriptor.reader import DescriptorReader
# from stem.descriptor import export
import sqlite3 as lite
import os.path
import os
# You can use this if you have pyliblzma...
#import contextlib
#import lzma
#import tarfile
import urllib
#import bz2
import subprocess
import Queue
from shutil import copyfileobj

def my_reporthook (chunk_number, max_size_chunk, total_size):
	if chunk_number == 0:
		print "Total download size: " + str(total_size)
		print "Progress bar"
		print "|**************************************************|"
		sys.stdout.write("|")
		sys.stdout.flush()
	elif chunk_number % round((total_size/max_size_chunk)/50) == 0:
		sys.stdout.write("#")
		sys.stdout.flush()
	if chunk_number * max_size_chunk >= total_size:
		sys.stdout.write("|")
		sys.stdout.flush()
		print " "

debug_level = 3 # With debug level of 0 no debug information is printed

scriptname = 'construct_database.py'
database_name_prefix = '../data/server_descriptors'
table_name = 'server_descriptors'
con = None

## Setup
if len(sys.argv) < 2:
	print "usage: python " + scriptname + " year-month"
	print "e.g., python " + scriptname + " 2012-02"
	sys.exit(1)
year_month = sys.argv[1]
url = "https://collector.torproject.org/archive/relay-descriptors/server-descriptors/server-descriptors-" + year_month + ".tar.xz"
file_name = "../data/server-descriptors-" + year_month + ".tar.xz"
file_name_tar = "../data/server-descriptors-" + year_month + ".tar"
database_name = database_name_prefix + '-' + year_month + '.sqlite'
database_csv_name = database_name_prefix + '-' + year_month + '_tmp.csv'

if os.path.exists(file_name):
	print "File exists :" + file_name
if os.path.exists(file_name_tar):
	print "File exists :" + file_name_tar
if os.path.exists(database_name):
	print "File exists :" + database_name



# Downloading file
if ((not os.path.exists(file_name)) and (not os.path.exists(file_name_tar)) and (not os.path.exists(database_name))):
	if debug_level >= 2:
			print "Starting download .."
	urllib.urlretrieve(url,file_name, my_reporthook)
	if debug_level >= 2:
			print ".. finished"

# Decompressing file
if ((not os.path.exists(file_name_tar)) and (not os.path.exists(database_name))):
	if debug_level >= 2:
		print "Starting decompression .."
		#~ with bz2.BZ2File(file_name, 'rb') as input:
			#~ with open(file_name_tar, 'wb') as output:
					#~ copyfileobj(input, output)
	# Requires pyliblzma:
	#~ with contextlib.closing(lzma.LZMAFile('test.tar.xz')) as xz:
		#~ with tarfile.open(fileobj=xz) as f:
			#~ f.extractall('.')
	#subprocess.check_call(["xz", "-d", file_name])
	subprocess.check_call("xz -dc " + file_name + " > " + file_name_tar, shell=True)
	if debug_level >= 2:
		print ".. finished."

# Creating the sqlite3 database and the view
if not os.path.exists(database_name):
	if debug_level >= 2:
		print "Creating the database and connecting to it .."
	con = lite.connect(database_name)
	sql_query = '\
	CREATE TABLE ' + table_name + '\
	(\
	address 			TEXT,\
	family 				TEXT,\
	fingerprint 		TEXT,\
	nickname 			TEXT,\
	published 			TEXT\
	);'
	if debug_level >= 3:
		print ".. with the following sql query: "
		print sql_query
	try:
		con.execute(sql_query)
	except lite.Error, e:
		print "Error %s:" % e.args[0]
		sys.exit(1)
	if debug_level >= 2:
			print ".. finished."
	# Create the view familyview
	if debug_level >= 2:
		print "Creating the view 'familyview' .."
	sql_query_familyview = "CREATE VIEW familyview AS SELECT nickname || '@' || address || ' ' || published AS hash, * FROM server_descriptors WHERE \
								 family != 'set([])' AND hash != 'nickname@address published'";
	if debug_level >= 3:
		print ".. with the following sql query .. "
		print sql_query_familyview
	try:
		con.execute(sql_query_familyview)
	except lite.Error, e:
		print "Error %s:" % e.args[0]
		sys.exit(1)
	if debug_level >= 2:
		print ".. finished."
	
else:
	if debug_level >= 2:
		print "Connecting to the database .."
	con = lite.connect(database_name)
	if debug_level >= 2:
		print ".. finished."

if os.path.exists(file_name_tar) and not os.path.exists(database_csv_name):
	with DescriptorReader(file_name_tar) as reader:
		with open(database_csv_name, 'a') as csv_file:
			csv_file.write("address,family,fingerprint,nickname,published\n")
			values_list = []
			insert_string = 'INSERT INTO ' + table_name + ' VALUES (?,?,?,?,?)'
			# i = 0
			# last_i = 0
			if debug_level >= 2:
				print "Reading the server descriptors .."
			for desc in reader:
				con.execute(insert_string, (str(desc.address), str(desc.family), str(desc.fingerprint), str(desc.nickname), str(desc.published)))
				
# finally:
if con:
	if debug_level >= 2:
		print "Closing the database .."
	con.commit()
	con.close()
	if debug_level >= 2:
		print ".. finished."
if os.path.exists(database_csv_name):
	os.remove(database_csv_name);
if os.path.exists(file_name):
	os.remove(file_name);
if os.path.exists(file_name_tar):
	os.remove(file_name_tar);
