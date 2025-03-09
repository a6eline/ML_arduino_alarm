
// THESE ARE NOTES FOR ME TO INVESTIGATE THE LIBRARY/EXAMPLES FOR MY OWN USE 

//==================================================================== micro_speech.ino ====================================================================

  //-------------------------FILE-OVERVIEW-------------------------------
    // setting up tensorflow lite micro interpreter to process audio
    // capturing audio input from the MP34DT06JTR microphone 
    // extracting relevent features like spectrogram
    // running inference using tensorflow lite model 
    // determining if a command word is recognized (yes or no)
    // responnding accodingly to the command word (changing LED colour/serial prints)


//---------------------------------------------------------------------PREPROCESSOR ----------------------------------------------------------------------------------------------

#include <TensorFlowLite.h>

#include "audio_provider.h"
#include "command_responder.h"
#include "feature_provider.h"
#include "main_functions.h"
#include "micro_features_micro_model_settings.h"
#include "micro_features_model.h"
#include "recognize_commands.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"

//---------------------------------------------------------------------GLOBALS----------------------------------------------------------------------------------------------

// ------------------------NAMESPACE-------------------------------
// these are pointers to key objects in the tensorflow lite micro system
namespace {
  const tflite::Model* model = nullptr;             // model            -> holds the tflite model, later used in setup()
  tflite::MicroInterpreter* interpreter = nullptr;  // interpreter      -> responsible for running the model
  TfLiteTensor* model_input = nullptr;              // model_input      -> holds the input tensor where we feed in audio features
  FeatureProvider* feature_provider = nullptr;      // feature_provider -> converts raw audio into spectrograms (features suitable for the model) 
  RecognizeCommands* recognizer = nullptr;          // recognizer       -> helps detect valid voice commands based on model output
  int32_t previous_time = 0;                        // previous_time    -> stores timestamp for last processed audio frame for the system to know when audio data ia vailable
  
  
  constexpr int kTensorArenaSize = 10 * 1024;         // tensor_arean     -> memory buffer for storing model inputs, outputs and intermediate values
                                                      // The size of this will depend on the model you're using, and may need to be determined by experimentation.    
  alignas(16) uint8_t tensor_arena[kTensorArenaSize]; // Keep aligned to 16 bytes for CMSIS 
  
  int8_t feature_buffer[kFeatureElementCount];      // feature_buffer   -> stores extracted audio features before they are fed into th emodel
  int8_t* model_input_buffer = nullptr;             // model_input_buffer -> used to pass feature data into the model
}  // namespace

//==================================================================== SETUP() ====================================================================

//---------------------------------------------------------------------voiceRecogitionSetup----------------------------------------------------------------------------------------------

// this function includes all the setup functions from micro_speech.ino in the micro_speech example
void voiceRecogitionSetup() {
  tflite::InitializeTarget(); // initialises the tensorflow lite micro system for the hardware

  //------------------------------------------MAPPING-MODEL---------------------------------------
  
  // Map the model into a usable data structure. This doesn't involve any copying or parsing, it's a very lightweight operation.
  model = tflite::GetModel(g_model); // g_model -> defined in micro_features_model.cpp
  if (model->version() != TFLITE_SCHEMA_VERSION) {  // checks compatibility with the TFLite version
    MicroPrintf(
        "Model provided is schema version %d not equal "
        "to supported version %d.",
        model->version(), TFLITE_SCHEMA_VERSION);
    return;
  }

  //----------------------------------------REGISTER-OPERATIONS------------------------------------

  // Pull in only the operation implementations we need.
  // This relies on a complete list of all the ops needed by this graph. An easier approach is to just use the AllOpsResolver, 
  // but this will incur some penalty in code space for op implementations that are not needed by this graph.
  //
  // tflite::AllOpsResolver resolver;
  // NOLINTNEXTLINE(runtime-global-variables)
  static tflite::MicroMutableOpResolver<4> micro_op_resolver;
  if (micro_op_resolver.AddDepthwiseConv2D() != kTfLiteOk) {  // DepthwiseConv2D  -> convolution layer
    return;
  } if (micro_op_resolver.AddFullyConnected() != kTfLiteOk) { // FullyConnected   -> dense layer
    return;
  } if (micro_op_resolver.AddSoftmax() != kTfLiteOk) {        // Softmax -> classification function
    return;
  } if (micro_op_resolver.AddReshape() != kTfLiteOk) {        // Reshape -> adjusting tensor shapes
    return;
  }

  //-------------------------------------------INTEPRETER-------------------------------------------

  // Build an interpreter to run the model with.
  static tflite::MicroInterpreter static_interpreter(
      model, micro_op_resolver, tensor_arena, kTensorArenaSize);
  interpreter = &static_interpreter;

  // Allocate memory from the tensor_arena for the model's tensors.
  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    MicroPrintf("AllocateTensors() failed");
    return;
  }

  // Get information about the memory area to use for the model's input.
  model_input = interpreter->input(0);
  if ((model_input->dims->size != 2) || (model_input->dims->data[0] != 1) ||
      (model_input->dims->data[1] !=
       (kFeatureSliceCount * kFeatureSliceSize)) ||
      (model_input->type != kTfLiteInt8)) {
    MicroPrintf("Bad input tensor parameters in model");
    return;
  }
  model_input_buffer = model_input->data.int8;
  
  //-------------------------------------------FEATURE-------------------------------------------

  // Prepare to access the audio spectrograms from a microphone or other source
  // that will provide the inputs to the neural network.
  // NOLINTNEXTLINE(runtime-global-variables)
  static FeatureProvider static_feature_provider(kFeatureElementCount, feature_buffer);
  feature_provider = &static_feature_provider; // FeatureProvider^ -> extracts audio files 


  //-------------------------------------------RECOGNIZE-COMMANDS-------------------------------------------

  static RecognizeCommands static_recognizer; // RecognizeCommands -> processes model ouputs
  recognizer = &static_recognizer;

  previous_time = 0;

  //-------------------------------------------MIC-RECORDING-------------------------------------------

  // start "recording" the audio from mic input
  TfLiteStatus init_status = InitAudioRecording();
  if (init_status != kTfLiteOk) {
    MicroPrintf("Unable to initialize audio");
    return;
  }
  MicroPrintf("Initialization complete");
}


//==================================================================== LOOP() ====================================================================

//---------------------------------------------------------------------GetLatestAudio----------------------------------------------------------------------------------------------

// voice_recognition.cpp --> Fetch the spectrogram for the current time.
void GetLatestAudio() {
  const int32_t current_time = LatestAudioTimestamp();
  int how_many_new_slices = 0;
  TfLiteStatus feature_status = feature_provider->PopulateFeatureData(
      previous_time, current_time, &how_many_new_slices);
  if (feature_status != kTfLiteOk) {
    MicroPrintf("Feature generation failed");
    return;
  }
  
  previous_time += how_many_new_slices * kFeatureSliceStrideMs;
  // If no new audio samples have been received since last time, don't bother running the network model.
  if (how_many_new_slices == 0) {
    return;
  }
} // RETURN int32_t PREVIOUS_TIME

//---------------------------------------------------------------------runModel----------------------------------------------------------------------------------------------

void RunModel() {
  // Copy feature buffer to input tensor
  for (int i = 0; i < kFeatureElementCount; i++) {
    model_input_buffer[i] = feature_buffer[i];
  }

  // Run the model on the spectrogram input and make sure it succeeds.
  TfLiteStatus invoke_status = interpreter->Invoke();
  if (invoke_status != kTfLiteOk) {
    MicroPrintf("Invoke failed");
    return;
  }
}

//---------------------------------------------------------------------runModel----------------------------------------------------------------------------------------------

void AnalyseOutput() {
  // Obtain a pointer to the output tensor
  TfLiteTensor* output = interpreter->output(0);
  // Determine whether a command was recognized based on the output of inference
  const char* found_command = nullptr;
  uint8_t score = 0;
  bool is_new_command = false;
  TfLiteStatus process_status = recognizer->ProcessLatestResults(
      output, current_time, &found_command, &score, &is_new_command);
  if (process_status != kTfLiteOk) {
    MicroPrintf("RecognizeCommands::ProcessLatestResults() failed");
    return;
  }
}


