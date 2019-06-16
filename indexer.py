#Spawn workers to create indexes based on VOTable files
import multiprocessing
from os import listdir
from math import ceil
import subprocess

dir_path   = "TAPFiles/"
index_path = "Index/"
indexers = []

#We need to figure out how many cores the current system has, and how many files we need to index
cpu_num  = multiprocessing.cpu_count()
print("Detected {} cores.".format(cpu_num))

file_names = listdir(dir_path)
file_num   = len(file_names)
print("Detected {} files in {} directory.".format(file_num, dir_path))

worker_num       = min(file_num,cpu_num)
files_per_worker = ceil(file_num /worker_num)
if worker_num * files_per_worker == file_num:
	print("Using {} workers, {} files per worker.".format(worker_num, files_per_worker))
else:
	print("Using {} workers, {} files per worker ({} file(s) on last worker).".format(worker_num, files_per_worker, file_num-((worker_num-1)*files_per_worker)))

start_file_id = 0
end_file_id   = files_per_worker
while start_file_id != end_file_id:
	#Spawn each worker with their associated files
	print(file_names[start_file_id:end_file_id])
	indexer = subprocess.Popen(['./create_index', 'Index/']+list(map(lambda x: dir_path+x, file_names[start_file_id:end_file_id])))
	#print("{},{}".format(start_file_id, end_file_id))
	start_file_id = start_file_id + files_per_worker if (start_file_id + files_per_worker <= file_num) else file_num
	end_file_id = end_file_id + files_per_worker if (end_file_id + files_per_worker <= file_num) else file_num

#Now, we wait for all processes to end
