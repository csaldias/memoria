#Spawn workers to search a given query over the full index DB
from os import listdir
from math import ceil
import subprocess, threading, multiprocessing, json

index_path     = "Index/"
search_threads = []
query          = "ALMA"
search_results = {}

def search_thread(thread_id, query, files):
	#print("Thread {}: Query={}".format(thread_id, query))
	#print("Thread {}: Files={}".format(thread_id, files))

	for file in files:
		#First, let's get the table name, service name (and VO name) out of the filename
		file_name = file.split(".")[0].split("/")[1]
		end_pos   = file_name.rfind("_")
		start_pos = file_name.find("_")
		if "ESO" in file_name:
			start_pos = file_name.find("_", start_pos+1, len(file_name))

		table_name = file_name[start_pos+1:end_pos]
		service_name = file_name[:start_pos]
		print("Thread {}: Querying File {} {}".format(thread_id, service_name, table_name))
		results = subprocess.check_output(['./search_index', query]+[file])
		results = json.loads(results)

		if not service_name in search_results: search_results[service_name] = {}
		search_results[service_name][table_name] = results
			

	

#We need to figure out how many cores the current system has, and how many files we need to index
cpu_num  = multiprocessing.cpu_count()
print("Detected {} cores.".format(cpu_num))

file_names = listdir(index_path)
file_num   = len(file_names)
print("Detected {} files in {} directory.".format(file_num, index_path))

worker_num       = min(file_num,cpu_num)
files_per_worker = ceil(file_num /worker_num)
if worker_num * files_per_worker == file_num:
	print("Using {} workers, {} files per worker.".format(worker_num, files_per_worker))
else:
	print("Using {} workers, {} files per worker ({} file(s) on last worker).".format(worker_num, files_per_worker, file_num-((worker_num-1)*files_per_worker)))

start_file_id = 0
end_file_id   = files_per_worker
thread_id     = 1

while start_file_id != end_file_id:
	#Spawn each worker with their associated files
	x = threading.Thread(target=search_thread, args=(thread_id, query, list(map(lambda x: index_path+x, file_names[start_file_id:end_file_id])),))
	search_threads.append(x)
	start_file_id = start_file_id + files_per_worker if (start_file_id + files_per_worker <= file_num) else file_num
	end_file_id = end_file_id + files_per_worker if (end_file_id + files_per_worker <= file_num) else file_num
	thread_id += 1

for x in search_threads:
	x.start()

#Now, we wait for all processes to end
for x in search_threads:
	x.join()

print(json.dumps(search_results, indent=4))