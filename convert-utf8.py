#-*- coding: utf-8 -*-
import os
import argparse
import codecs

def is_code(data, code):
	try:
		data.decode(code)
	except:
		return False
	return True

def convert_code(file, target_code):
	if not os.path.exists(file):
		return

	with open (file, "rb") as f:
		file_data = f.read()
		f.close()

	if is_code(file_data, target_code):
		return

	codes = ['gbk']
	original_code = None

	for code in codes:
		try:
			file_data.decode(code)
		except:
			continue
		original_code = code
		break

	if original_code:
		print("convert into %s %s" % (target_code, file))
		with codecs.open(file, "r", original_code) as sourceFile:
			contents = sourceFile.read()
			sourceFile.close()
		with codecs.open(file, "w", target_code) as targetFile:
			targetFile.write(contents)
			targetFile.close()

def convert_lf(file):
	if not os.path.exists(file):
		return

	with open (file, "r") as f:
		file_data = f.read()
		target_data = file_data.replace("\r\n", "\n")
		f.close()

		if file_data != target_data:
			print("convert into lf %s" % (file))
			with open (file, "w") as f:
				f.write(target_data)
				f.close()

def convert(path):
	for f in os.listdir(path):
		full_path = os.path.join(path, f)
		if os.path.isfile(full_path):
			exts = ['.cpp', '.c', '.h']
			name, ext = os.path.splitext(full_path)
			if ext in exts:
				convert_code(full_path, 'utf-8')
				convert_lf(full_path)
		else:
			convert(full_path)

def main():
	parser = argparse.ArgumentParser()
	parser.add_argument('--path', type=str, help='path to convert', default='KBase,KRender,KEngine,KEditor')
	args = parser.parse_args()
	paths = args.path.split(",")
	for path in paths:
		convert(path)

if __name__ == '__main__':
	try:
		main()
	except Exception as e:
		print('ERROR:', e)
		import traceback
		print(traceback.format_exc())