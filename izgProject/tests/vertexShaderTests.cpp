#include <tests/catch.hpp>
#include <tests/testCommon.hpp>

#include <iostream>
#include <string.h>
#include <sstream>
#include<functional>
#include<map>

#include <algorithm>
#include <numeric>

#include <student/gpu.hpp>
#include <framework/framebuffer.hpp>

#include <glm/gtc/matrix_transform.hpp>

using namespace tests;

bool operator==(Attribute const&a,Attribute const&b){
  return a.v4 == b.v4;
}

bool operator==(InVertex const&a,InVertex const&b){
  for(size_t i=0;i<maxAttributes;++i)
    if(!(a.attributes[i] == b.attributes[i]))return false;
  return a.gl_VertexID == b.gl_VertexID;
}

void printInVertices(std::vector<InVertex>const&inV,GPUContext const&ctx){
  std::stringstream ss;
  ss << "      gl_VertexID: {";
  bool first = true;
  for(auto const&v:inV){
    if(first)first = false;
    else ss << ",";
    ss << v.gl_VertexID;
  }
  ss << "}" << std::endl;

  for(size_t a=0;a<maxAttributes;++a){
    if(ctx.vao.vertexAttrib[a].type == AttributeType::EMPTY)continue;
    ss << "      attribut" << a << ": {";
    if(ctx.vao.vertexAttrib[a].bufferData){
      bool first = true;
      for(auto const&v:inV){
        if(first)first = false;
        else ss << ",";
        if(ctx.vao.vertexAttrib[a].type==AttributeType::FLOAT) ss <<     v.attributes[a].v1;
        if(ctx.vao.vertexAttrib[a].type==AttributeType::VEC2 ) ss << str(v.attributes[a].v2);
        if(ctx.vao.vertexAttrib[a].type==AttributeType::VEC3 ) ss << str(v.attributes[a].v3);
        if(ctx.vao.vertexAttrib[a].type==AttributeType::VEC4 ) ss << str(v.attributes[a].v4);
        //ss << str(v.attributes[a].v4);
      }
    }
    ss << "}" << std::endl;
  }
  std::cerr << ss.str();
}

std::vector<InVertex>computeExpectedInVertices(GPUContext const&ctx,size_t n){
  std::vector<InVertex>result;
  auto const&vao = ctx.vao;
  for(uint32_t i=0;i<n;++i){
    InVertex inV;
    OutVertex outV;
    if(vao.indexBuffer){
      if(vao.indexType == IndexType::UINT8 )inV.gl_VertexID = ((uint8_t *)vao.indexBuffer)[i];
      if(vao.indexType == IndexType::UINT16)inV.gl_VertexID = ((uint16_t*)vao.indexBuffer)[i];
      if(vao.indexType == IndexType::UINT32)inV.gl_VertexID = ((uint32_t*)vao.indexBuffer)[i];
    }else{
      inV.gl_VertexID = i;
    }
    for(uint32_t a=0;a<maxAttributes;++a){
      auto const&at = ctx.vao.vertexAttrib[a];
      if(!at.bufferData)continue;
      if(at.type == AttributeType::FLOAT)inV.attributes[a].v1 = *(float    *)(((uint8_t*)at.bufferData)+at.stride*inV.gl_VertexID+at.offset);
      if(at.type == AttributeType::VEC2 )inV.attributes[a].v2 = *(glm::vec2*)(((uint8_t*)at.bufferData)+at.stride*inV.gl_VertexID+at.offset);
      if(at.type == AttributeType::VEC3 )inV.attributes[a].v3 = *(glm::vec3*)(((uint8_t*)at.bufferData)+at.stride*inV.gl_VertexID+at.offset);
      if(at.type == AttributeType::VEC4 )inV.attributes[a].v4 = *(glm::vec4*)(((uint8_t*)at.bufferData)+at.stride*inV.gl_VertexID+at.offset);
    }
    result.push_back(inV);
  }
  return result;
}

void printVertexShaderInput(GPUContext const&ctx,size_t n){
  std::cerr << std::endl;
  std::cerr << "    Vertexy, kter?? do??ly do vertex shaderu:" << std::endl;
  printInVertices(inVertices,ctx);
  std::cerr << std::endl;
  std::cerr << "    Vertexy, kter?? m??ly doj??t do vertex shaderu:" << std::endl;
  printInVertices(computeExpectedInVertices(ctx,n),ctx);
}

void printCode(GPUContext const&ctx,size_t n){
  std::cerr << std::endl;
  std::cerr << "    K??d vypad?? takto:" << std::endl;

  std::map<IndexType,std::string>indexTypes;
  indexTypes[IndexType::UINT8 ] = "uint8_t"  ;
  indexTypes[IndexType::UINT16] = "uint16_t" ;
  indexTypes[IndexType::UINT32] = "uint32_t" ;

  size_t nofVertices = 0;

  if(ctx.vao.indexBuffer){
    std::cerr << "    " << indexTypes[ctx.vao.indexType] << " indices[] = {";
    bool first=true;
    for(uint32_t i=0;i<n;++i){
      if(first)first=false;
      else std::cerr << ",";
      size_t vid=0;
      if(ctx.vao.indexType == IndexType::UINT8 )vid = (uint32_t)(((uint8_t *)ctx.vao.indexBuffer)[i]);
      if(ctx.vao.indexType == IndexType::UINT16)vid = (uint32_t)(((uint16_t*)ctx.vao.indexBuffer)[i]);
      if(ctx.vao.indexType == IndexType::UINT32)vid = (uint32_t)(((uint32_t*)ctx.vao.indexBuffer)[i]);

      std::cerr << vid;
      nofVertices = glm::max(nofVertices,vid);
    }
    std::cerr << "};" << std::endl;
  }else{
    nofVertices = n-1;
  }


  std::map<void const*,std::string>vbos;
  for(size_t a=0;a<maxAttributes;++a){
    auto const&at=ctx.vao.vertexAttrib[a];
    if(!at.bufferData)continue;
    std::stringstream ss;
    ss << vbos.size();
    vbos[at.bufferData] = "buffer" + ss.str();

    if(at.offset%sizeof(float)==0){
      std::cerr << "    float buffer" + ss.str() << "[] = {";
      float*ptr = (float*)at.bufferData;
      bool first=true;
      while(ptr<(float*)(((uint8_t*)at.bufferData)+at.offset+at.stride*(nofVertices)+(uint64_t)at.type*sizeof(float))){
        if(first)first=false;
        else std::cerr << ",";
        std::cerr << ptr[0];
        ptr++;
      }
      std::cerr << "};" << std::endl;
    }
  }


  std::cerr << std::endl;
  std::cerr << "    GPUContext ctx;" << std::endl;

  for(size_t a=0;a<maxAttributes;++a){
    auto const&at=ctx.vao.vertexAttrib[a];
    if(at.bufferData){
      std::cerr << "    ctx.vao.vertexAttrib["<<a<<"].bufferData = "<<vbos[at.bufferData]<<";" << std::endl;
      std::cerr << "    ctx.vao.vertexAttrib["<<a<<"].type       = "<<str(at.type) <<";" << std::endl;
      std::cerr << "    ctx.vao.vertexAttrib["<<a<<"].stride     = "<<at.stride          <<"; // krok v bajtech" << std::endl;
      std::cerr << "    ctx.vao.vertexAttrib["<<a<<"].offset     = "<<at.offset          <<"; // offset v bajtech" << std::endl;
    }
  }
  std::cerr << std::endl;
  if(ctx.vao.indexBuffer){
    std::cerr << "    ctx.vao.indexBuffer = indices;" << std::endl;
    std::cerr << "    ctx.vao.indexType   = "<<str(ctx.vao.indexType)<<";" << std::endl;
  }

  std::cerr << "    drawTriangles(ctx,"<<n<<");" << std::endl;
  std::cerr << std::endl;
  std::cerr << std::endl;
}

void fragmentShaderEmpty(OutFragment&,InFragment const&,Uniforms const&){}

SCENARIO("00"){
  std::cerr << "00 - vertex shader invocation counter tests" << std::endl;


  auto framebuffer = std::make_shared<Framebuffer>(100,100);
  GPUContext ctx;
  ctx.frame              = framebuffer->getFrame();
  ctx.prg.vertexShader   = vertexShaderDump;
  ctx.prg.fragmentShader = fragmentShaderEmpty;

  {
    inVertices.clear();
    uint32_t N = 3;
    drawTriangles(ctx,N);

    auto expected = computeExpectedInVertices(ctx,N);
    if(inVertices.size() != expected.size()){
      std::cerr << R".(
      TEST SELHAL!

      Kdy?? se zavol?? funkce drawTriangles (drawTrianglesImpl), m??l by se zavolat vertex shader.
      kdy?? se zavol?? funkce takto: drawTriangles(ctx,)."<< N <<R".(), znamen?? to, ??e by se m??l vertex shader pustit )."<< N << R".(x.
      Vertex Shader se nach??z?? v parametru ctx.prg.vertexShader, co?? je ukazatel na funkci.
      Lze jej zavolat: ctx.prg.vertexShader("parametry").

      Funkce drawTriangles p??ibli??n?? odpov??d?? funkc??m glDrawArrays(GL_TRIANGLES,...) a glDrawElements(GL_TRIANGLES,...) z OpenGL.

      O??ek??v??ny po??et zavol??n??: )." << N << R".(
      Kolikr??t se zavolal: )." << inVertices.size() << R".(

      N??pov??da k implementaci:

      void drawTrianglesImpl(GPUContext& ctx,uint32_t nofVertices){

        for(...){//smy??ka p??es vrcholy
          InVertex  inVertex; // vrchol, co vstupuje do vertex shader
          OutVertex outVertex;// vrchol, co leze ven z vertex shaderu 
          ctx.prg.vertexShader(outVertex,inVertex,ctx.prg.uniforms);
        }

      }

      ).";
      
      REQUIRE(false);
    }
  }

  {
    inVertices.clear();
    uint32_t N=1800;
    drawTriangles(ctx,N);
    auto expected = computeExpectedInVertices(ctx,N);
    if(inVertices.size() != expected.size()){
      std::cerr << R".(
      vertex shader by se m??l zavolat )." << N << R".(x, kdy?? se zavol?? funkce drawTriangles(ctx,)."<< N << R".().

      O??ek??v??ny po??et zavol??n??: )." << N << R".(
      Kolikr??t se zavolal: )." << inVertices.size() << R".(
      ).";

      //printVertexShaderInput(ctx,N);

      REQUIRE(false);
    }
  }
}

SCENARIO("01"){
  std::cerr << "01 - vertex shader, gl_VertexID (no indexing)" << std::endl;

  uint32_t nofV = 9;
  inVertices.clear();

  GPUContext ctx;
  auto framebuffer = std::make_shared<Framebuffer>(100,100);
  ctx.frame              = framebuffer->getFrame();
  ctx.prg.vertexShader   = vertexShaderDump;
  ctx.prg.fragmentShader = fragmentShaderEmpty;

  drawTriangles(ctx,nofV);

  framebuffer = nullptr;

  auto expected = computeExpectedInVertices(ctx,nofV);

  if(inVertices != expected){
    std::cerr << R".(
    Vrcholy chod?? do vertex shaderu ????slovan??.
    ????slo vrcholu je ulo??eno v polo??ce gl_VertexID ve struktu??e InVertex.
    Kdy?? nen?? zapnut?? indexov??n?? (v tomto testu nen??), tak je ????slo vrcholu rovno ????slu zavol??n?? vertex shaderu.

    Prom??nn?? gl_VertexID odpov??da prom??nn?? gl_VertexId z GLSL.)."<<std::endl;

    printVertexShaderInput(ctx,nofV);

    std::cerr << R".(
    N??pov??da k implementaci:

    void drawTrianglesImpl(GPUContext& ctx,uint32_t nofVertices){

      for(...){//smy??ka p??es vrcholy
        InVertex  inVertex; // vrchol, co vstupuje do vertex shader
        OutVertex outVertex;// vrchol, co leze ven z vertex shaderu 
        inVertex.gl_VertexID = ??;
        ctx.prg.vertexShader(outVertex,inVertex,ctx.prg.uniforms);
      }

    }

    ).";

    REQUIRE(false);
  }
}

SCENARIO("02"){
  std::cerr << "02 - vertex shader, gl_VertexID (32bit indexing)" << std::endl;

  inVertices.clear();

  std::vector<uint32_t> indices = {0,1,2,2,1,3};
  auto const indicesSize = indices.size() * sizeof(decltype(indices)::value_type);

  GPUContext ctx;
  auto framebuffer = std::make_shared<Framebuffer>(100,100);
  ctx.frame              = framebuffer->getFrame();
  ctx.vao.indexBuffer    = indices.data();
  ctx.vao.indexType      = IndexType::UINT32;
  ctx.prg.vertexShader   = vertexShaderDump ;
  ctx.prg.fragmentShader = fragmentShaderEmpty;

  drawTriangles(ctx,static_cast<uint32_t>(indices.size()));

  auto expected = computeExpectedInVertices(ctx,indices.size());

  if(inVertices != expected){

    printCode(ctx,indices.size());

    std::cerr << R".(
    Zavolali jsme kreslen?? pro ??est vrchol??.
    T??ch ??et vrchol?? by m??lo m??t spr??vn?? ????slo (gl_VertexId), kter?? je rovno hodnot?? z indexa??n??ho bufferu.

    Vrcholy chod?? do vertex shaderu ????slovan??.
    ????slo vrcholu je ulo??eno v polo??ce gl_VertexID ve struktu??e InVertex.
    Kdy?? je zapnut?? indexov??n?? (v tomto testu je), tak je ????slo vrcholu rovno hodnot?? v indexa??n??m bufferu.
    V tomto testu se p??edpokl??d?? 32 bitov?? index.
    Buffer s indexy je v prom??nn?? ctx.vao.indexBuffer.
    Typ indexu (8b,16b,32b) je v prom??nn?? ctx.prg.indexType.
    Nezapome??te spr??vn?? adresovat indexa??n?? buffer.
    Jedn?? se o tzn. "pointer arithmetic".

    Mus??te si uv??domit, ??e ctx.vao.indexBuffer ukazuje na za????tek indexa??n??ho bufferu.
    Indexa??n?? buffer obsahuje indexy, kter?? maj?? v??echny stejnou velikost, kter?? m????e b??t 8b, 16b nebo 32 bitov??.

    Polo??ka z indexa??n??ho bufferu je vybr??na ????slem invokace vertex shaderu.

    Na buffer je mo??n?? se nahl????et jako na pole:
    uint32_t*pole;

    ????slo vrcholu gl_VertexID je potom rovno pole[????slo invokace vertex shaderu].

    Indexing je vypnut??, pokud je pointer ctx.vao.indexBuffer == nullptr.).";

    printVertexShaderInput(ctx,indices.size());

    std::cerr << R".(
    N??pov??da k implementaci:

    uint32_t computeVertexID(VertexArray const&vao,uint32_t shaderInvocation){
      if(!vao.indexBuffer)return shaderInvocation;

      if(vao.indexType == IndexType::UIN32){
        uint32_t*ind = (uint32_t*)vao.indexBuffer;
        return ind[shaderInvocation];
      }
      ...
    }

    void drawTrianglesImpl(GPUContext& ctx,uint32_t nofVertices){

      for(...){//smy??ka p??es vrcholy
        ...
        InVertex  inVertex; // vrchol, co vstupuje do vertex shader
        inVertex.gl_VertexID = computeVertexID(ctx.vao,...);
        ...
      }

    }


    ).";

    REQUIRE(false);
  }
}



SCENARIO("03"){
  std::cerr << "03 - vertex shader, gl_VertexID (16bit indexing)" << std::endl;

  inVertices.clear();

  std::vector<uint16_t> indices = {0,1,2,2,1,3};
  auto const indicesSize = indices.size() * sizeof(decltype(indices)::value_type);

  auto framebuffer = std::make_shared<Framebuffer>(100,100);
  GPUContext ctx;
  ctx.frame              = framebuffer->getFrame();
  ctx.vao.indexBuffer    = indices.data();
  ctx.vao.indexType      = IndexType::UINT16;
  ctx.prg.vertexShader   = vertexShaderDump ;
  ctx.prg.fragmentShader = fragmentShaderEmpty;

  drawTriangles(ctx,static_cast<uint32_t>(indices.size()));

  auto expected = computeExpectedInVertices(ctx,indices.size());

  if(inVertices != expected){
    std::cerr << R".(
    Stejn?? poh??dka jako u 32bit indexa??n??ho bufferu, a?? na to, ??e se pou????v?? 16 bitov?? index.
    Tak??e nastala tato situace:)." << std::endl;
    
    printCode(ctx,indices.size());

    printVertexShaderInput(ctx,indices.size());

    REQUIRE(false);
  }
}

SCENARIO("04"){
  std::cerr << "04 - vertex shader, gl_VertexID (8bit indexing)" << std::endl;

  inVertices.clear();

  std::vector<uint8_t> indices = {0,1,2,2,1,3};
  auto const indicesSize = indices.size() * sizeof(decltype(indices)::value_type);

  auto framebuffer = std::make_shared<Framebuffer>(100,100);
  GPUContext ctx;
  ctx.frame              = framebuffer->getFrame();
  ctx.vao.indexBuffer    = indices.data();
  ctx.vao.indexType      = IndexType::UINT8;
  ctx.prg.vertexShader   = vertexShaderDump ;
  ctx.prg.fragmentShader = fragmentShaderEmpty;

  drawTriangles(ctx,static_cast<uint32_t>(indices.size()));

  auto expected = computeExpectedInVertices(ctx,indices.size());

  if(inVertices != expected){
    std::cerr << R".(
    Stejn?? poh??dka jako u 32bit indexa??n??ho bufferu, a?? na to, ??e se pou????v?? 8 bitov?? index.
    Tak??e nastala tato situace:)." << std::endl;
    
    printCode(ctx,indices.size());

    printVertexShaderInput(ctx,indices.size());

    REQUIRE(false);
  }
}



SCENARIO("05"){
  std::cerr << "05 - vertex shader, uniforms" << std::endl;

  auto framebuffer = std::make_shared<Framebuffer>(100,100);
  GPUContext ctx;
  ctx.frame              = framebuffer->getFrame();
  ctx.prg.vertexShader   = vertexShaderDump ;
  ctx.prg.fragmentShader = fragmentShaderEmpty;

  struct Case{
    char const*typeName;
    char const*component;
    uint32_t   location;
    Uniform    value;
    std::function<bool(Uniform,Uniform)>cmp;
  };

  Case cases[] = {
    Case{"float"    ,"v1",0,[](){Uniform a;a.v1=3.f                                    ;return a;}(),[](Uniform a,Uniform b){return a.v1==b.v1;}},
    Case{"glm::vec2","v2",1,[](){Uniform a;a.v2=glm::vec2       (2.0f, 3.0f           );return a;}(),[](Uniform a,Uniform b){return a.v2==b.v2;}},
    Case{"glm::vec3","v3",0,[](){Uniform a;a.v3=glm::vec3       (5.0f, 4.0f, 3.0f     );return a;}(),[](Uniform a,Uniform b){return a.v3==b.v3;}},
    Case{"glm::vec4","v4",3,[](){Uniform a;a.v4=glm::vec4       (8.0f,-1.0f, 3.0f,7.0f);return a;}(),[](Uniform a,Uniform b){return a.v4==b.v4;}},
    Case{"glm::mat4","m4",4,[](){Uniform a;a.m4=glm::perspective(1.0f, 1.3f,13.3f,9.8f);return a;}(),[](Uniform a,Uniform b){return a.m4==b.m4;}},
  };

  uint32_t nofCases = sizeof(cases) / sizeof(Case);

  for(uint32_t i=0;i<nofCases;++i)
  {
    char const*typeName  = cases[i].typeName ;
    char const*component = cases[i].component;
    auto       location  = cases[i].location ;
    auto       value     = cases[i].value    ;
    auto       cmp       = cases[i].cmp      ;
    ctx.prg.uniforms.uniform[location] = cases[i].value;
    drawTriangles(ctx,3);

    if(!cmp(unif.uniform[location],value)){
      std::cerr << R".(
      Nastala tato situace:
      ctx.prg.vertexShader = n??jaky vertex shader;
      ctx.prg.uniforms.uniforms[)."<<location<<R".(].)."<< component << R".( = )."<<value.v1<<R".(;
      drawTriangles(ctx,3);

      U??ivatel si nastavit jednu uniformn?? prom??nnou a pustil vykreslov??n?? a ??ek??, ??e se mu ta uniformn?? prom??nn?? dostane do vertex shaderu.

      U??ivatel funkce drawTriangles ??asto chce pro sv?? vykreslov??n?? nastavit n??jak?? konstanty (uniformy), t??eba matice.
      Tyto uniformy jsou ulo??en?? ve struktu??e Uniforms. Tady konkr??tn?? v prom??nn?? ctx.prg.uniforms.

      U??ivatel se tady v tomto p????pad?? rozhodl nastavit jednu uniformn?? prom??nnou na lokaci: )." << location << R".(.
      Je to prom??nn?? typu )."<< typeName << R".(

      Vertex Shader by m??l tuto hodnotu obdr??et.

      Pozn??mka:
      Uniformn?? prom??nn?? jsou v tomto projektu reprezentov??ny jako Union.
      Union je datov?? typ z C++ a je podobn?? skruku??e, ale m?? obvykle men???? velikost.

      struct S{
        float   a;
        uint8_t b;
      };

      union U{
        float   a;
        uint8_t b;
      };

      Oba typy maj?? polo??ky "a", "b", ale obvykle sizeof(S) != sizeof(U).
      U unionu je pam??t polo??ek p??ekryt?? p??es sebe a m?? velikost nejv??t???? polo??ky.

      Tzn, v p????klad?? v????e bude obvykle sizeof(U) == sizeof(float).
      Ale sizeof(S) == sizeof(float) + sizeof(uint8_t).

      To tak?? znamen??, ??e si takto ??lov??k p??episuje data
      U u;
      u.b = 32;
      u.a = 1.337; // t??mto jsme si p??epsali "b"

      VIZ o unionech: https://en.cppreference.com/w/cpp/language/union

      K uniformn??m prom??nn?? se v OpenGL v????e mnoho funkc?? glUnifom*

      ).";

      REQUIRE(false);
    }
  }

}


SCENARIO("06"){
  std::cerr << "06 - vertex shader, attributes" << std::endl;

  inVertices.clear();

  std::vector<float> vert = {0.f,1.f,2.f,3.f,4.f,5.f,6.f,7.f,8.f,9.f,10.f,11.f};

  auto framebuffer = std::make_shared<Framebuffer>(100,100);
  GPUContext ctx;
  ctx.frame                          = framebuffer->getFrame();
  ctx.vao.vertexAttrib[0].bufferData = vert.data();
  ctx.vao.vertexAttrib[0].type       = AttributeType::FLOAT;
  ctx.vao.vertexAttrib[0].stride     = sizeof(float);
  ctx.vao.vertexAttrib[0].offset     = 0;
  ctx.prg.vertexShader               = vertexShaderDump ;
  ctx.prg.fragmentShader             = fragmentShaderEmpty;

  drawTriangles(ctx,static_cast<uint32_t>(vert.size()));

  auto expected = computeExpectedInVertices(ctx,vert.size());

  if(inVertices != expected){
    std::cerr << R".(

    Tento test kontroluje, jestli se do vertex shaderu pos??laj?? spr??vn?? data z vertex bufferu.)."<<std::endl;

    printCode(ctx,vert.size());

    std::cerr << R".(
    U??ivatel si vytvo??il buffer float?? a nastavil jeden vertex attribut.
    Pot?? pustil vykreslov??n?? a o??ek??v??, ??e se mu ve vstupn??m vrcholu (InVertex) do vertex shaderu objev?? spr??vn?? data.).";

    printVertexShaderInput(ctx,vert.size());

    REQUIRE(false);
  }
}


SCENARIO("07"){
  std::cerr << "07 - vertex shader, attributes, offset" << std::endl;

  auto const offsetInFloats = 3;
  inVertices.clear();

  std::vector<float> vert = {0.f,1.f,2.f,3.f,4.f,5.f,6.f,7.f,8.f,9.f,10.f,11.f};
  auto vertSize = vert.size()*sizeof(decltype(vert)::value_type);

  auto framebuffer = std::make_shared<Framebuffer>(100,100);
  GPUContext ctx;
  ctx.frame                          = framebuffer->getFrame();
  ctx.vao.vertexAttrib[0].bufferData = vert.data();
  ctx.vao.vertexAttrib[0].type       = AttributeType::FLOAT;
  ctx.vao.vertexAttrib[0].stride     = sizeof(float);
  ctx.vao.vertexAttrib[0].offset     = sizeof(float)*offsetInFloats;
  ctx.prg.vertexShader               = vertexShaderDump ;
  ctx.prg.fragmentShader             = fragmentShaderEmpty;

  auto vertToDraw = vert.size()-offsetInFloats;
  drawTriangles(ctx,static_cast<uint32_t>(vertToDraw));

  auto expected = computeExpectedInVertices(ctx,vertToDraw);

  if(inVertices != expected){
    std::cerr << R".(

    Tento test kontroluje, jestli se do vertex shaderu pos??laj?? spr??vn?? data z vertex bufferu kdy?? se pou??ije offset.)."<<std::endl;

    printCode(ctx,vertToDraw);

    std::cerr << R".(
    U??ivatel si vytvo??il buffer float?? a nastavil jeden vertex attribut, ke kter??mu si zvolil offset.
    Pot?? pustil vykreslov??n?? a o??ek??v??, ??e se mu ve vstupn??m vrcholu (InVertex) do vertex shaderu objev?? spr??vn?? data s vyu??it??m offsetem.)." << std::endl;

    printVertexShaderInput(ctx,vertToDraw);

    REQUIRE(false);
  }
}


SCENARIO("08"){
  std::cerr << "08 - vertex shader, attributes, stride" << std::endl;

  auto const strideInFloats = 4;
  inVertices.clear();

  std::vector<float> vert = {0.f,1.f,2.f,3.f,4.f,5.f,6.f,7.f,8.f,9.f,10.f,11.f};
  auto vertSize = vert.size()*sizeof(decltype(vert)::value_type);

  auto framebuffer = std::make_shared<Framebuffer>(100,100);
  GPUContext ctx;
  ctx.frame                          = framebuffer->getFrame();
  ctx.vao.vertexAttrib[0].bufferData = vert.data();
  ctx.vao.vertexAttrib[0].type       = AttributeType::FLOAT;
  ctx.vao.vertexAttrib[0].stride     = sizeof(float)*strideInFloats;
  ctx.vao.vertexAttrib[0].offset     = 0;
  ctx.prg.vertexShader               = vertexShaderDump ;
  ctx.prg.fragmentShader             = fragmentShaderEmpty;

  auto vertToDraw = vert.size()/strideInFloats;
  drawTriangles(ctx,static_cast<uint32_t>(vertToDraw));

  auto expected = computeExpectedInVertices(ctx,vertToDraw);

  if(inVertices != expected){
    std::cerr << R".(

    Tento test kontroluje, jestli se do vertex shaderu pos??laj?? spr??vn?? data z vertex bufferu kdy?? se pou??ije krok (stride).)."<<std::endl;

    printCode(ctx,vertToDraw);

    std::cerr << R".(
    U??ivatel si vytvo??il buffer float?? a nastavil jeden vertex attribut, ke kter??mu si zvolil stride.
    Pot?? pustil vykreslov??n?? a o??ek??v??, ??e se mu ve vstupn??m vrcholu (InVertex) do vertex shaderu objev?? spr??vn?? data s vyu??it??m krokem.)." << std::endl;

    printVertexShaderInput(ctx,vertToDraw);

    REQUIRE(false);
  }

}


SCENARIO("09"){
  std::cerr << "09 - vertex shader, attributes, offset, stride" << std::endl;

  auto const strideInFloats = 4;
  auto const offsetInFloats = 2;
  inVertices.clear();

  std::vector<float> vert = {0.f,1.f,2.f,3.f,4.f,5.f,6.f,7.f,8.f,9.f,10.f,11.f};
  auto vertSize = vert.size()*sizeof(decltype(vert)::value_type);

  auto framebuffer = std::make_shared<Framebuffer>(100,100);
  GPUContext ctx;
  ctx.frame                          = framebuffer->getFrame();
  ctx.vao.vertexAttrib[0].bufferData = vert.data();
  ctx.vao.vertexAttrib[0].type       = AttributeType::FLOAT;
  ctx.vao.vertexAttrib[0].stride     = sizeof(float)*strideInFloats;
  ctx.vao.vertexAttrib[0].offset     = sizeof(float)*offsetInFloats;
  ctx.prg.vertexShader               = vertexShaderDump ;
  ctx.prg.fragmentShader             = fragmentShaderEmpty;

  auto vertToDraw = vert.size()/strideInFloats;
  drawTriangles(ctx,static_cast<uint32_t>(vertToDraw));

  auto expected = computeExpectedInVertices(ctx,vertToDraw);

  if(inVertices != expected){
    std::cerr << R".(

    Tento test kontroluje, jestli se do vertex shaderu pos??laj?? spr??vn?? data z vertex bufferu kdy?? se pou??ije offset a krok.)." << std::endl;

    printCode(ctx,vertToDraw);

    std::cerr << R".(
    U??ivatel si vytvo??il buffer float?? a nastavil jeden vertex attribut, ke kter??mu si zvolil offset i stride.
    Pot?? pustil vykreslov??n?? a o??ek??v??, ??e se mu ve vstupn??m vrcholu (InVertex) do vertex shaderu objev?? spr??vn?? data s vyu??it??m krokem.)." << std::endl;

    printVertexShaderInput(ctx,vertToDraw);

    REQUIRE(false);
  }

}


SCENARIO("10"){
  std::cerr << "10 - vertex shader, attributes, offset, stride, multiple buffers" << std::endl;

  GPUContext ctx;
  auto framebuffer = std::make_shared<Framebuffer>(100,100);
  ctx.frame = framebuffer->getFrame();

  uint8_t b0[400];
  uint8_t b1[400];

  memset(b0,0,sizeof(b0));
  memset(b1,0,sizeof(b1));

  ((float*)(b0+2))[0] = 100.f;
  ((float*)(b0+2))[1] = 101.f;
  ((float*)(b0+2))[2] = 102.f;
  ((float*)(b0+2))[3] = 103.f;
  ((float*)(b0+2))[4] = 104.f;
  ((float*)(b0+2))[5] = 105.f;
  ((float*)(b0+2))[6] = 106.f;

  ((float*)(b1+7))[ 0] = 10.f;
  ((float*)(b1+7))[ 1] = 11.f;
  ((float*)(b1+7))[ 2] = 12.f;
  ((float*)(b1+7))[ 3] = 13.f;
  ((float*)(b1+7))[ 4] =  0.f;
  ((float*)(b1+7))[ 5] = 14.f;
  ((float*)(b1+7))[ 6] = 15.f;
  ((float*)(b1+7))[ 7] = 16.f;
  ((float*)(b1+7))[ 8] = 17.f;
  ((float*)(b1+7))[ 9] =  0.f;
  ((float*)(b1+7))[10] = 18.f;
  ((float*)(b1+7))[11] = 19.f;
  ((float*)(b1+7))[12] = 20.f;
  ((float*)(b1+7))[13] = 21.f;


  ctx.vao.vertexAttrib[5].bufferData = b0;
  ctx.vao.vertexAttrib[5].type       = AttributeType::VEC3;
  ctx.vao.vertexAttrib[5].stride     = sizeof(float)*2;
  ctx.vao.vertexAttrib[5].offset     = 2;

  ctx.vao.vertexAttrib[3].bufferData = b1;
  ctx.vao.vertexAttrib[3].type       = AttributeType::VEC4;
  ctx.vao.vertexAttrib[3].stride     = sizeof(float)*5;
  ctx.vao.vertexAttrib[3].offset     = 7;

  ctx.prg.vertexShader   = vertexShaderDump ;
  ctx.prg.fragmentShader = fragmentShaderEmpty;

  inVertices.clear();
  drawTriangles(ctx,3);

  auto expected = computeExpectedInVertices(ctx,3);
  if(inVertices != expected){
    std::cerr << R".(

    Tento test kontroluje, jestli se do vertex shaderu pos??laj?? spr??vn?? data z vertex bufferu kdy?? se pou??ije offset a krok a v??cero buffer??.

    Nastala tato situace, kdy m?? u??ivatel dva buffery (b0, b1), ve kter??ch jsou ulo??eny vertex attributy.
    V buferu b1 jsou ulo??eny vec3 s offsetem 2 bajty a krokem pouze 8 (tak??e se komponenty p??ekr??vaj??).
    V buffer b0 jsou ulo??eny vec4 s offsetem 7 bajt?? a krokem 20 (tak??e je mezi jednotliv??mi vec4 mezera).
    U??ivatel vyu??il 3. a 5. attribut. K??d vypad?? takto:)." << std::endl;

    printCode(ctx,3);

    printVertexShaderInput(ctx,3);

    REQUIRE(false);
  }

}


SCENARIO("11"){
  std::cerr << "11 - vertex shader, attributes, offset, stride, indexing" << std::endl;
  GPUContext ctx;
  auto framebuffer = std::make_shared<Framebuffer>(100,100);
  ctx.frame = framebuffer->getFrame();

  std::vector<glm::vec2> vert = {glm::vec2(-1.f,-1.f),glm::vec2(1.f,-1.f),glm::vec2(-1.f,1.f),glm::vec2(1.f,1.f)};
  auto vertSize = vert.size()*sizeof(decltype(vert)::value_type);

  std::vector<uint16_t> indices = {0,1,2,2,1,3};
  auto indicesSize = indices.size()*sizeof(decltype(indices)::value_type);

  ctx.vao.vertexAttrib[0].bufferData = vert.data();
  ctx.vao.vertexAttrib[0].type       = AttributeType::VEC2;
  ctx.vao.vertexAttrib[0].stride     = sizeof(glm::vec2);
  ctx.vao.vertexAttrib[0].offset     = 0;

  ctx.vao.indexBuffer = indices.data();
  ctx.vao.indexType   = IndexType::UINT16;

  ctx.prg.vertexShader   = vertexShaderDump   ;
  ctx.prg.fragmentShader = fragmentShaderEmpty;


  inVertices.clear();
  drawTriangles(ctx,static_cast<uint32_t>(indices.size()));

  auto expected = computeExpectedInVertices(ctx,indices.size());

  if(inVertices != expected){

    std::cerr << R".(

    Tento test kontroluje, jestli se do vertex shaderu pos??laj?? spr??vn?? data z vertex bufferu kdy?? se pou??ije indexing.)."<<std::endl;

    printCode(ctx,indices.size());

    std::cerr << R".(
    U??ivatel si vytvo??il vertex buffer a index buffer a cht??l vykreslit ??tverec pomoc?? 2 troj??heln??k??.)." << std::endl;

    printVertexShaderInput(ctx,indices.size());

    REQUIRE(false);
  }
}

