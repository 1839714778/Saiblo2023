from keras import models
from keras import layers
from keras import optimizers
import math
import tensorflow as tf
def MyLoss(y_true,y_pred):
	L1=tf.reduce_sum(tf.math.square(y_true[:,:1]-y_pred[:,:1]))
	L2=-tf.reduce_sum(y_true[:,1:]*tf.math.log(y_pred[:,1:]),-1)
	return L1+L2

network=models.load_model('model.h5')
network.compile(optimizer=optimizers.Adam(),loss=MyLoss,metrics=[])
network.save('model.h5',include_optimizer=False)
