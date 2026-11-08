#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <math.h>
#include <string.h>
#include "wire.h"
static int error(const char *msg){PyErr_SetString(PyExc_ValueError,msg);return 0;}
static const char *type_of(PyObject *f){PyObject *t=PyDict_GetItemString(f,"type");return t?PyUnicode_AsUTF8(t):NULL;}
static int repeated(PyObject *f){PyObject *v=PyDict_GetItemString(f,"repeated");return v&&PyObject_IsTrue(v);}
static int wire_type(const char *t){if(!strcmp(t,"string")||!strcmp(t,"bytes")||!strcmp(t,"message"))return 2;if(!strcmp(t,"double")||!strcmp(t,"fixed64")||!strcmp(t,"sfixed64"))return 1;if(!strcmp(t,"float")||!strcmp(t,"fixed32")||!strcmp(t,"sfixed32"))return 5;return 0;}
static int put(iibin_buffer *b,const void *d,size_t n){return iibin_append(b,d,n)||error("Message exceeds 16 MiB or allocation failed");}
static int varint(iibin_buffer *b,uint64_t v){return iibin_write_varint(b,v)||error("Message exceeds 16 MiB or allocation failed");}
static int encode_message(PyObject *,PyObject *,iibin_buffer *,int);
static PyObject *decode_message(PyObject *,const unsigned char *,size_t,int);
static int number(PyObject *v,const char *t,uint64_t *out){
    if(!strcmp(t,"bool")){if(!PyBool_Check(v))return error("Expected bool");*out=v==Py_True;return 1;}
    if(!PyLong_Check(v)||PyBool_Check(v))return error("Expected integer");
    int uns=!strcmp(t,"uint32")||!strcmp(t,"uint64")||!strcmp(t,"fixed32")||!strcmp(t,"fixed64");
    if(uns){*out=PyLong_AsUnsignedLongLong(v);if(PyErr_Occurred())return 0;if((strstr(t,"32"))&&*out>UINT32_MAX)return error("Unsigned integer out of range");}
    else {long long n=PyLong_AsLongLong(v);if(PyErr_Occurred())return 0;if(strstr(t,"32")&&(n<INT32_MIN||n>INT32_MAX))return error("Signed integer out of range");*out=(uint64_t)n;}
    return 1;
}
static int encode_value(PyObject *f,PyObject *v,iibin_buffer *b,int depth){
    const char *t=type_of(f);if(!t)return 0;int w=wire_type(t);
    if(w==2){
        if(!strcmp(t,"message")){iibin_buffer nested={0};int ok=encode_message(PyDict_GetItemString(f,"schema"),v,&nested,depth+1);if(ok)ok=varint(b,nested.size)&&put(b,nested.data,nested.size);iibin_buffer_free(&nested);return ok;}
        if(!strcmp(t,"string")){if(!PyUnicode_Check(v))return error("Expected string");Py_ssize_t n;const char *s=PyUnicode_AsUTF8AndSize(v,&n);if(!s)return 0;return varint(b,(uint64_t)n)&&put(b,s,(size_t)n);}
        Py_buffer view;if(PyObject_GetBuffer(v,&view,PyBUF_SIMPLE)<0)return 0;int ok=varint(b,view.len)&&put(b,view.buf,view.len);PyBuffer_Release(&view);return ok;
    }
    uint64_t bits=0;
    if(!strcmp(t,"float")||!strcmp(t,"double")){if(PyBool_Check(v)||(!PyFloat_Check(v)&&!PyLong_Check(v)))return error("Expected finite number");double d=PyFloat_AsDouble(v);if(PyErr_Occurred())return 0;if(!isfinite(d))return error("Non-finite number");if(w==5){float x=(float)d;if(!isfinite(x))return error("Float out of range");uint32_t u;memcpy(&u,&x,4);bits=u;}else memcpy(&bits,&d,8);}
    else if(!number(v,t,&bits))return 0;
    if(w==0){if(!strncmp(t,"sint",4))bits=(bits<<1)^((bits>>63)?UINT64_MAX:0);return varint(b,bits);}
    unsigned char buf[8];size_t count=w==5?4:8;for(size_t i=0;i<count;i++)buf[i]=(unsigned char)(bits>>(8*i));return put(b,buf,count);
}
static int encode_message(PyObject *schema,PyObject *obj,iibin_buffer *b,int depth){
    if(depth>32)return error("Message nesting exceeds 32");if(!PyDict_Check(obj))return error("Message must be a dictionary");
    PyObject *key,*value;Py_ssize_t pos=0;
    while(PyDict_Next(obj,&pos,&key,&value))if(!PyDict_Contains(schema,key))return error("Unknown message field");
    pos=0;
    while(PyDict_Next(schema,&pos,&key,&value)){
        PyObject *v=PyDict_GetItemWithError(obj,key);if(!v){if(PyErr_Occurred())return 0;continue;}
        const char *t=type_of(value);if(!t)return 0;uint64_t tag=(PyLong_AsUnsignedLong(PyDict_GetItemString(value,"id"))<<3)|wire_type(t);
        if(PyErr_Occurred())return 0;
        if(repeated(value)){if(!PyList_Check(v)&&!PyTuple_Check(v))return error("Repeated field must be a list or tuple");Py_ssize_t n=PySequence_Size(v);for(Py_ssize_t i=0;i<n;i++){PyObject *item=PySequence_GetItem(v,i);if(!item)return 0;int ok=varint(b,tag)&&encode_value(value,item,b,depth);Py_DECREF(item);if(!ok)return 0;}}
        else if(!varint(b,tag)||!encode_value(value,v,b,depth))return 0;
    }return 1;
}
static PyObject *decode_value(PyObject *f,const unsigned char *data,size_t n,size_t *pos,int depth){
    const char *t=type_of(f);if(!t)return NULL;int w=wire_type(t);uint64_t u=0;
    if(w==2){uint64_t len;if(!iibin_read_varint(data,n,pos,&len)||len>n-*pos){error("Truncated length-delimited field");return NULL;}const unsigned char *s=data+*pos;*pos+=(size_t)len;
        if(!strcmp(t,"string"))return PyUnicode_DecodeUTF8((const char*)s,(Py_ssize_t)len,"strict");
        if(!strcmp(t,"bytes"))return PyBytes_FromStringAndSize((const char*)s,(Py_ssize_t)len);
        return decode_message(PyDict_GetItemString(f,"schema"),s,(size_t)len,depth+1);
    }
    if(w==0){if(!iibin_read_varint(data,n,pos,&u)){error("Truncated or overflowing varint");return NULL;}}
    else {size_t count=w==5?4:8;if(n-*pos<count){error("Truncated fixed-width field");return NULL;}for(size_t i=0;i<count;i++)u|=((uint64_t)data[(*pos)++])<<(8*i);}
    if(!strcmp(t,"bool"))return PyBool_FromLong(u!=0);
    if(!strcmp(t,"float")){uint32_t v=(uint32_t)u;float x;memcpy(&x,&v,4);if(!isfinite(x)){error("Non-finite float");return NULL;}return PyFloat_FromDouble(x);}
    if(!strcmp(t,"double")){double x;memcpy(&x,&u,8);if(!isfinite(x)){error("Non-finite double");return NULL;}return PyFloat_FromDouble(x);}
    if(!strncmp(t,"sint",4)){if(strstr(t,"32")&&u>UINT32_MAX){error("sint32 out of range");return NULL;}return PyLong_FromLongLong((int64_t)((u>>1)^(uint64_t)-(int64_t)(u&1)));}
    if(!strcmp(t,"int32")){
        if(u>INT32_MAX && u<(uint64_t)(int64_t)INT32_MIN){error("int32 out of range");return NULL;}
        return PyLong_FromLong((int32_t)u);
    }
    if(!strcmp(t,"sfixed32"))return PyLong_FromLong((int32_t)u);
    if(!strcmp(t,"uint32")||!strcmp(t,"fixed32")){if(u>UINT32_MAX){error("uint32 out of range");return NULL;}return PyLong_FromUnsignedLong((uint32_t)u);}
    if(!strcmp(t,"int64")||!strcmp(t,"sfixed64"))return PyLong_FromLongLong((int64_t)u);
    return PyLong_FromUnsignedLongLong(u);
}
static int skip(const unsigned char *d,size_t n,size_t *p,int w){uint64_t v;if(w==0)return iibin_read_varint(d,n,p,&v);if(w==2){if(!iibin_read_varint(d,n,p,&v)||v>n-*p)return 0;*p+=(size_t)v;return 1;}size_t k=w==1?8:w==5?4:0;if(!k||n-*p<k)return 0;*p+=k;return 1;}
static PyObject *decode_message(PyObject *schema,const unsigned char *d,size_t n,int depth){
    if(depth>32){error("Message nesting exceeds 32");return NULL;}
    PyObject *result=PyDict_New();if(!result)return NULL;size_t p=0;
    while(p<n){uint64_t tag;if(!iibin_read_varint(d,n,&p,&tag)||tag<8||tag>>3>=((uint64_t)1<<29)){error("Invalid field tag");goto fail;}int w=tag&7;PyObject *key,*field,*found=NULL,*name=NULL;Py_ssize_t it=0;
        while(PyDict_Next(schema,&it,&key,&field)){unsigned long id=PyLong_AsUnsignedLong(PyDict_GetItemString(field,"id"));if(id==(tag>>3)){found=field;name=key;break;}}
        if(!found){if(!skip(d,n,&p,w)){error("Invalid unknown field");goto fail;}continue;}
        const char *t=type_of(found);if(!t)goto fail;int expected=wire_type(t);
        int packed=w==2&&expected!=2&&repeated(found);size_t packed_end=0;
        if(packed){uint64_t len;if(!iibin_read_varint(d,n,&p,&len)||len>n-p){error("Truncated packed field");goto fail;}if(!len)continue;packed_end=p+(size_t)len;}
        PyObject *list=NULL;
        if(repeated(found)){list=PyDict_GetItemWithError(result,name);if(!list){list=PyList_New(0);if(!list)goto fail;int rc=PyDict_SetItem(result,name,list);Py_DECREF(list);if(rc<0)goto fail;list=PyDict_GetItem(result,name);}}
        if(packed){while(p<packed_end){PyObject *v=decode_value(found,d,packed_end,&p,depth);if(!v)goto fail;int rc=PyList_Append(list,v);Py_DECREF(v);if(rc<0)goto fail;}continue;}
        if(w!=expected){error("Field wire type mismatch");goto fail;}
        PyObject *v=decode_value(found,d,n,&p,depth);if(!v)goto fail;int rc=list?PyList_Append(list,v):PyDict_SetItem(result,name,v);Py_DECREF(v);if(rc<0)goto fail;
    }return result;
fail:Py_DECREF(result);return NULL;
}
static PyObject *encode(PyObject *self,PyObject *args){(void)self;PyObject *schema,*obj;if(!PyArg_ParseTuple(args,"O!O!",&PyDict_Type,&schema,&PyDict_Type,&obj))return NULL;iibin_buffer b={0};int ok=encode_message(schema,obj,&b,0);PyObject *out=ok?PyBytes_FromStringAndSize((char*)b.data,(Py_ssize_t)b.size):NULL;iibin_buffer_free(&b);return out;}
static PyObject *decode(PyObject *self,PyObject *args){(void)self;PyObject *schema;Py_buffer view;if(!PyArg_ParseTuple(args,"O!y*",&PyDict_Type,&schema,&view))return NULL;if(view.len>IIBIN_MAX_BYTES){PyBuffer_Release(&view);error("Message exceeds 16 MiB");return NULL;}PyObject *out=decode_message(schema,view.buf,view.len,0);PyBuffer_Release(&view);return out;}
static PyMethodDef methods[]={{"encode",encode,METH_VARARGS,"Encode schema message in native C."},{"decode",decode,METH_VARARGS,"Decode schema message in native C."},{NULL,NULL,0,NULL}};
static struct PyModuleDef module={PyModuleDef_HEAD_INIT,"_native",NULL,-1,methods,NULL,NULL,NULL,NULL};
PyMODINIT_FUNC PyInit__native(void){return PyModule_Create(&module);}
