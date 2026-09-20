#include "juceshim.h"
#define JUCE_DSP_H_INCLUDED
#include "LCRExtractor_local.h"
#include <cstdio>
#include <cmath>
#include <random>
static double rms(const std::vector<float>& v,int a,int b){double s=0;for(int i=a;i<b;++i)s+=(double)v[i]*v[i];return std::sqrt(s/(b-a));}
int main(){
    const double sr=48000.0; StereoSTFTExtractor ex; ex.prepare(sr); ex.setExtractionRange(20.0f,22000.0f);
    const int lat=ex.getLatencySamples(); const int N=lat*12;
    std::vector<float> C(N),L(N),R(N),inL(N),inR(N);
    std::mt19937 rng(7); std::uniform_real_distribution<float> d(-1.f,1.f);
    for(int i=0;i<N;++i){
        double t=(double)i/sr;
        double mid  = 0.5*std::sin(2*M_PI*440.0*t);        // zentriert
        double left = 0.5*std::sin(2*M_PI*1230.0*t);       // hart links
        inL[i]=(float)(mid+left); inR[i]=(float)mid;
    }
    for(int i=0;i<N;++i){ float lo,c,ro; ex.processSample(inL[i],inR[i],0.5f,lo,c,ro); C[i]=c;L[i]=lo;R[i]=ro; }
    int a=lat*4,b=N;
    printf("Center RMS : %.4f  (erwartet ~0.354 = der 440er)\n", rms(C,a,b));
    printf("L-only RMS : %.4f  (erwartet ~0.354 = der 1230er)\n", rms(L,a,b));
    printf("R-only RMS : %.4f  (erwartet ~0)\n", rms(R,a,b));

    // Test 2: dekorreliertes Rauschen darf NICHT in die Mitte
    StereoSTFTExtractor e2; e2.prepare(sr); e2.setExtractionRange(20.0f,22000.0f);
    std::vector<float> C2(N),L2(N),R2(N);
    for(int i=0;i<N;++i){ float lo,c,ro; e2.processSample(0.3f*d(rng),0.3f*d(rng),0.5f,lo,c,ro); C2[i]=c;L2[i]=lo;R2[i]=ro; }
    printf("\nDekorreliertes Rauschen -> Center RMS: %.4f  (erwartet nahe 0)\n", rms(C2,a,b));

    // Test 3: reines Mono muss vollstaendig in die Mitte
    StereoSTFTExtractor e3; e3.prepare(sr); e3.setExtractionRange(20.0f,22000.0f);
    std::vector<float> C3(N),L3(N);
    for(int i=0;i<N;++i){ float x=0.3f*d(rng); float lo,c,ro; e3.processSample(x,x,0.5f,lo,c,ro); C3[i]=c;L3[i]=lo; }
    printf("Mono-Rauschen -> Center RMS: %.4f, L-only RMS: %.4f  (erwartet ~0.173 / ~0)\n", rms(C3,a,b), rms(L3,a,b));
    return 0;
}
