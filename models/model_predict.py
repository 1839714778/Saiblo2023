import os 
os.environ['CUDA_VISIBLE_DEVICES']='-1'
os.environ['TF_CPP_MIN_LOG_LEVEL']='3'
from keras import models
import tensorflow as tf
import numpy as np 
# import keras
tf.compat.v1.disable_eager_execution()
# print(tf.executing_eagerly())
model_a0=models.load_model('/home/monkey/Saiblo2023/models/model/model_a0.h5')
model_a1=models.load_model('/home/monkey/Saiblo2023/models/model/model_a1.h5')
# network.summary()
np.random.seed(20050114)

def predict_a0(input,valid,applyDir):
	output=model_a0.predict(np.asarray([input]),verbose=0)[0]
	valid=np.asarray(valid)
	output=np.asarray(output)
	tmp=output[1:]
	tmp=tmp*valid
	for i in range(len(valid)):
		if valid[i]<=0.001:
			valid[i]=0.001
	if(applyDir):
		tmp=tmp*0.75+np.random.dirichlet(valid*0.3,1)[0]
	dlist=list(np.concatenate((output[:1].astype('float64'),tmp.astype('float64'))))
	result=[]
	for x in dlist:
		result.append(x.item())
	return result

def predict_a1(input,valid,applyDir):
	output=model_a1.predict(np.asarray([input]),verbose=0)[0]
	valid=np.asarray(valid)
	output=np.asarray(output)
	tmp=output[1:]
	tmp=tmp*valid
	for i in range(len(valid)):
		if valid[i]<=0.001:
			valid[i]=0.001
	if(applyDir):
		tmp=tmp*0.75+np.random.dirichlet(valid*0.3,1)[0]
	dlist=list(np.concatenate((output[:1].astype('float64'),tmp.astype('float64'))))
	result=[]
	for x in dlist:
		result.append(x.item())
	return result
