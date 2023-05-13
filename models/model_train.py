from keras import models
from keras import layers
from keras import optimizers
import math
import tensorflow as tf
import numpy as np
def MyLoss(y_true,y_pred):
	L1=tf.reduce_sum(tf.math.square(y_true[:,:1]-y_pred[:,:1]))
	L2=-tf.reduce_sum(y_true[:,1:]*tf.math.log(y_pred[:,1:]),-1)
	return L1+L2

def readData(file):
	data=[]
	labels=[]
	with open(file,'r') as f:
		for line in f.readlines():
			a=np.asarray(list(map(float,line.split())))
			data.append(a[:1995])
			labels.append(a[1995:])
	data=np.asarray(data)
	labels=np.asarray(labels)
	return data,labels

def getWinAcc(y_true,y_pred):
	print(y_true.shape,y_pred.shape)
	a,b=0,0
	f=open('temp.txt','w')
	for i in range(len(y_true)):
		print(y_true[i][0],y_pred[i][0],file=f)
		if (y_true[i][0]>=0)==(y_pred[i][0]>=0):
			a+=1
		b+=1
	return a/b


data,labels=readData('/home/monkey/Saiblo2023/models/data.txt')
network=models.load_model('/home/monkey/Saiblo2023/models/model/model_a1.h5')
network.compile(optimizer=optimizers.Adam(),loss=MyLoss,metrics=[])
network.fit(data,labels,epochs=10,validation_split=0.2)
print('win acc=',getWinAcc(network.predict(data),labels))

network.save('/home/monkey/Saiblo2023/models/model.h5',include_optimizer=False)