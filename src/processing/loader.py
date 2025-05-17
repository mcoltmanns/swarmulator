import dask.dataframe as dd
import sys

path = sys.argv[1]
static_path = path + ".static"
dynamic_path = path + ".dynamic"

dynamic_frame = dd.read_csv(dynamic_path, sep='|')
print('loaded')
print(dynamic_frame.columns)
print(dynamic_frame['time'].max().compute())
