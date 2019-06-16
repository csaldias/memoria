#Spawn workers to search a given query over the full index DB
from os import listdir
from math import ceil
import subprocess, threading, multiprocessing

index_path     = "Index/"
searchers      = []
query          = "ALMA"
search_results = {}

def search_thread(thread_id, query, files):
	print("Thread {}: Query={}".format(thread_id, query))
	print("Thread {}: Files={}".format(thread_id, files))
	#for file in files:
		#indexer = subprocess.Popen(['./args']+list(map(lambda x: index_path+x, file_names[start_file_id:end_file_id])))
	

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
	searchers.append(x)
	x.start()

	start_file_id = start_file_id + files_per_worker if (start_file_id + files_per_worker <= file_num) else file_num
	end_file_id = end_file_id + files_per_worker if (end_file_id + files_per_worker <= file_num) else file_num
	thread_id += 1

#Now, we wait for all processes to end
