#!/bin/bash

./caffe-dynamic-dilation/.build_release/tools/caffe.bin train --solver=solver.prototxt --weights=./model/init.caffemodel --gpu=1  2>&1 | tee log.txt

echo "Done."
