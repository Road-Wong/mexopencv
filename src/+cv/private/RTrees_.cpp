/**
 * @file RTrees_.cpp
 * @brief mex interface for cv::ml::RTrees
 * @ingroup ml
 * @author Kota Yamaguchi, Amro
 * @date 2012, 2015
 */
#include "mexopencv.hpp"
#include "mexopencv_ml.hpp"
#include "opencv2/ml.hpp"
using namespace std;
using namespace cv;
using namespace cv::ml;

// Persistent objects
namespace {
/// Last object id to allocate
int last_id = 0;
/// Object container
map<int,Ptr<RTrees> > obj_;
}

/**
 * Main entry called from Matlab
 * @param nlhs number of left-hand-side arguments
 * @param plhs pointers to mxArrays in the left-hand-side
 * @param nrhs number of right-hand-side arguments
 * @param prhs pointers to mxArrays in the right-hand-side
 */
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    // Check the number of arguments
    nargchk(nrhs>=2 && nlhs<=2);

    // Argument vector
    vector<MxArray> rhs(prhs, prhs+nrhs);
    int id = rhs[0].toInt();
    string method(rhs[1].toString());

    // Constructor is called. Create a new object from argument
    if (method == "new") {
        nargchk(nrhs==2 && nlhs<=1);
        obj_[++last_id] = RTrees::create();
        plhs[0] = MxArray(last_id);
        mexLock();
        return;
    }

    // Big operation switch
    Ptr<RTrees> obj = obj_[id];
    if (obj.empty())
        mexErrMsgIdAndTxt("mexopencv:error", "Object not found id=%d", id);
    if (method == "delete") {
        nargchk(nrhs==2 && nlhs==0);
        obj_.erase(id);
        mexUnlock();
    }
    else if (method == "clear") {
        nargchk(nrhs==2 && nlhs==0);
        obj->clear();
    }
    else if (method == "load") {
        nargchk(nrhs>=3 && (nrhs%2)==1 && nlhs==0);
        string objname;
        bool loadFromString = false;
        for (int i=3; i<nrhs; i+=2) {
            string key(rhs[i].toString());
            if (key == "ObjName")
                objname = rhs[i+1].toString();
            else if (key == "FromString")
                loadFromString = rhs[i+1].toBool();
            else
                mexErrMsgIdAndTxt("mexopencv:error",
                    "Unrecognized option %s", key.c_str());
        }
        //obj_[id] = RTrees::load(rhs[2].toString());
        obj_[id] = (loadFromString ?
            Algorithm::loadFromString<RTrees>(rhs[2].toString(), objname) :
            Algorithm::load<RTrees>(rhs[2].toString(), objname));
    }
    else if (method == "save") {
        nargchk(nrhs==3 && nlhs<=1);
        string fname(rhs[2].toString());
        if (nlhs > 0) {
            // write to memory, and return string
            FileStorage fs(fname, FileStorage::WRITE + FileStorage::MEMORY);
            if (!fs.isOpened())
                mexErrMsgIdAndTxt("mexopencv:error", "Failed to open file");
            fs << obj->getDefaultName() << "{";
            obj->write(fs);
            fs << "}";
            plhs[0] = MxArray(fs.releaseAndGetString());
        }
        else
            // write to disk
            obj->save(fname);
    }
    else if (method == "empty") {
        nargchk(nrhs==2 && nlhs<=1);
        plhs[0] = MxArray(obj->empty());
    }
    else if (method == "getDefaultName") {
        nargchk(nrhs==2 && nlhs<=1);
        plhs[0] = MxArray(obj->getDefaultName());
    }
    else if (method == "getVarCount") {
        nargchk(nrhs==2 && nlhs<=1);
        plhs[0] = MxArray(obj->getVarCount());
    }
    else if (method == "isClassifier") {
        nargchk(nrhs==2 && nlhs<=1);
        plhs[0] = MxArray(obj->isClassifier());
    }
    else if (method == "isTrained") {
        nargchk(nrhs==2 && nlhs<=1);
        plhs[0] = MxArray(obj->isTrained());
    }
    else if (method == "train") {
        nargchk(nrhs>=4 && (nrhs%2)==0 && nlhs<=1);
        vector<MxArray> dataOptions;
        int flags = 0;
        for (int i=4; i<nrhs; i+=2) {
            string key(rhs[i].toString());
            if (key == "Data")
                dataOptions = rhs[i+1].toVector<MxArray>();
            else if (key == "Flags")
                flags = rhs[i+1].toInt();
            else if (key == "RawOutput")
                UPDATE_FLAG(flags, rhs[i+1].toBool(), StatModel::RAW_OUTPUT);
            else if (key == "PredictSum")
                UPDATE_FLAG(flags, rhs[i+1].toBool(), DTrees::PREDICT_SUM);
            else if (key == "PredictMaxVote")
                UPDATE_FLAG(flags, rhs[i+1].toBool(), DTrees::PREDICT_MAX_VOTE);
            else
                mexErrMsgIdAndTxt("mexopencv:error",
                    "Unrecognized option %s", key.c_str());
        }
        Ptr<TrainData> data;
        if (rhs[2].isChar())
            data = loadTrainData(rhs[2].toString(),
                dataOptions.begin(), dataOptions.end());
        else
            data = createTrainData(
                rhs[2].toMat(CV_32F),
                rhs[3].toMat(rhs[3].isInt32() ? CV_32S : CV_32F),
                dataOptions.begin(), dataOptions.end());
        bool b = obj->train(data, flags);
        plhs[0] = MxArray(b);
    }
    else if (method == "calcError") {
        nargchk(nrhs>=4 && (nrhs%2)==0 && nlhs<=2);
        vector<MxArray> dataOptions;
        bool test = false;
        for (int i=4; i<nrhs; i+=2) {
            string key(rhs[i].toString());
            if (key == "Data")
                dataOptions = rhs[i+1].toVector<MxArray>();
            else if (key == "TestError")
                test = rhs[i+1].toBool();
            else
                mexErrMsgIdAndTxt("mexopencv:error",
                    "Unrecognized option %s", key.c_str());
        }
        Ptr<TrainData> data;
        if (rhs[2].isChar())
            data = loadTrainData(rhs[2].toString(),
                dataOptions.begin(), dataOptions.end());
        else
            data = createTrainData(
                rhs[2].toMat(CV_32F),
                rhs[3].toMat(rhs[3].isInt32() ? CV_32S : CV_32F),
                dataOptions.begin(), dataOptions.end());
        Mat resp;
        float err = obj->calcError(data, test, (nlhs>1 ? resp : noArray()));
        plhs[0] = MxArray(err);
        if (nlhs>1)
            plhs[1] = MxArray(resp);
    }
    else if (method == "predict") {
        nargchk(nrhs>=3 && (nrhs%2)==1 && nlhs<=2);
        int flags = 0;
        for (int i=3; i<nrhs; i+=2) {
            string key(rhs[i].toString());
            if (key == "Flags")
                flags = rhs[i+1].toInt();
            else if (key == "RawOutput")
                UPDATE_FLAG(flags, rhs[i+1].toBool(), StatModel::RAW_OUTPUT);
            else if (key == "CompressedInput")
                UPDATE_FLAG(flags, rhs[i+1].toBool(), StatModel::COMPRESSED_INPUT);
            else if (key == "PreprocessedInput")
                UPDATE_FLAG(flags, rhs[i+1].toBool(), StatModel::PREPROCESSED_INPUT);
            else if (key == "PredictAuto") {
                //UPDATE_FLAG(flags, rhs[i+1].toBool(), DTrees::PREDICT_AUTO);
                UPDATE_FLAG(flags, !rhs[i+1].toBool(), DTrees::PREDICT_SUM);
                UPDATE_FLAG(flags, !rhs[i+1].toBool(), DTrees::PREDICT_MAX_VOTE);
            }
            else if (key == "PredictSum")
                UPDATE_FLAG(flags, rhs[i+1].toBool(), DTrees::PREDICT_SUM);
            else if (key == "PredictMaxVote")
                UPDATE_FLAG(flags, rhs[i+1].toBool(), DTrees::PREDICT_MAX_VOTE);
            else
                mexErrMsgIdAndTxt("mexopencv:error",
                    "Unrecognized option %s", key.c_str());
        }
        Mat samples(rhs[2].toMat(CV_32F)),
            results;
        float f = obj->predict(samples, results, flags);
        plhs[0] = MxArray(results);
        if (nlhs>1)
            plhs[1] = MxArray(f);
    }
    else if (method == "getNodes") {
        nargchk(nrhs==2 && nlhs<=1);
        plhs[0] = toStruct(obj->getNodes());
    }
    else if (method == "getRoots") {
        nargchk(nrhs==2 && nlhs<=1);
        plhs[0] = MxArray(obj->getRoots());
    }
    else if (method == "getSplits") {
        nargchk(nrhs==2 && nlhs<=1);
        plhs[0] = toStruct(obj->getSplits());
    }
    else if (method == "getSubsets") {
        nargchk(nrhs==2 && nlhs<=1);
        plhs[0] = MxArray(obj->getSubsets());
    }
    else if (method == "getVarImportance") {
        nargchk(nrhs==2 && nlhs<=1);
        plhs[0] = MxArray(obj->getVarImportance());
    }
    else if (method == "getVotes") {
        nargchk(nrhs>=3 && (nrhs%2)==1 && nlhs<=1);
        int flags = 0;
        for (int i=3; i<nrhs; i+=2) {
            string key(rhs[i].toString());
            if (key == "Flags")
                flags = rhs[i+1].toInt();
            else if (key == "RawOutput")
                UPDATE_FLAG(flags, rhs[i+1].toBool(), StatModel::RAW_OUTPUT);
            else if (key == "CompressedInput")
                UPDATE_FLAG(flags, rhs[i+1].toBool(), StatModel::COMPRESSED_INPUT);
            else if (key == "PreprocessedInput")
                UPDATE_FLAG(flags, rhs[i+1].toBool(), StatModel::PREPROCESSED_INPUT);
            else if (key == "PredictAuto") {
                //UPDATE_FLAG(flags, rhs[i+1].toBool(), DTrees::PREDICT_AUTO);
                UPDATE_FLAG(flags, !rhs[i+1].toBool(), DTrees::PREDICT_SUM);
                UPDATE_FLAG(flags, !rhs[i+1].toBool(), DTrees::PREDICT_MAX_VOTE);
            }
            else if (key == "PredictSum")
                UPDATE_FLAG(flags, rhs[i+1].toBool(), DTrees::PREDICT_SUM);
            else if (key == "PredictMaxVote")
                UPDATE_FLAG(flags, rhs[i+1].toBool(), DTrees::PREDICT_MAX_VOTE);
            else
                mexErrMsgIdAndTxt("mexopencv:error",
                    "Unrecognized option %s", key.c_str());
        }
        Mat samples(rhs[2].toMat(CV_32F)),
            results;
        obj->getVotes(samples, results, flags);
        plhs[0] = MxArray(results);
    }
    else if (method == "get") {
        nargchk(nrhs==3 && nlhs<=1);
        string prop(rhs[2].toString());
        if (prop == "CVFolds")
            plhs[0] = MxArray(obj->getCVFolds());
        else if (prop == "MaxCategories")
            plhs[0] = MxArray(obj->getMaxCategories());
        else if (prop == "MaxDepth")
            plhs[0] = MxArray(obj->getMaxDepth());
        else if (prop == "MinSampleCount")
            plhs[0] = MxArray(obj->getMinSampleCount());
        else if (prop == "Priors")
            plhs[0] = MxArray(obj->getPriors());
        else if (prop == "RegressionAccuracy")
            plhs[0] = MxArray(obj->getRegressionAccuracy());
        else if (prop == "TruncatePrunedTree")
            plhs[0] = MxArray(obj->getTruncatePrunedTree());
        else if (prop == "Use1SERule")
            plhs[0] = MxArray(obj->getUse1SERule());
        else if (prop == "UseSurrogates")
            plhs[0] = MxArray(obj->getUseSurrogates());
        else if (prop == "ActiveVarCount")
            plhs[0] = MxArray(obj->getActiveVarCount());
        else if (prop == "CalculateVarImportance")
            plhs[0] = MxArray(obj->getCalculateVarImportance());
        else if (prop == "TermCriteria")
            plhs[0] = MxArray(obj->getTermCriteria());
        else
            mexErrMsgIdAndTxt("mexopencv:error",
                "Unrecognized property %s", prop.c_str());
    }
    else if (method == "set") {
        nargchk(nrhs==4 && nlhs==0);
        string prop(rhs[2].toString());
        if (prop == "CVFolds")
            obj->setCVFolds(rhs[3].toInt());
        else if (prop == "MaxCategories")
            obj->setMaxCategories(rhs[3].toInt());
        else if (prop == "MaxDepth")
            obj->setMaxDepth(rhs[3].toInt());
        else if (prop == "MinSampleCount")
            obj->setMinSampleCount(rhs[3].toInt());
        else if (prop == "Priors")
            obj->setPriors(rhs[3].toMat());
        else if (prop == "RegressionAccuracy")
            obj->setRegressionAccuracy(rhs[3].toFloat());
        else if (prop == "TruncatePrunedTree")
            obj->setTruncatePrunedTree(rhs[3].toBool());
        else if (prop == "Use1SERule")
            obj->setUse1SERule(rhs[3].toBool());
        else if (prop == "UseSurrogates")
            obj->setUseSurrogates(rhs[3].toBool());
        else if (prop == "ActiveVarCount")
            obj->setActiveVarCount(rhs[3].toInt());
        else if (prop == "CalculateVarImportance")
            obj->setCalculateVarImportance(rhs[3].toBool());
        else if (prop == "TermCriteria")
            obj->setTermCriteria(rhs[3].toTermCriteria());
        else
            mexErrMsgIdAndTxt("mexopencv:error",
                "Unrecognized property %s", prop.c_str());
    }
    else
        mexErrMsgIdAndTxt("mexopencv:error",
            "Unrecognized operation %s", method.c_str());
}
