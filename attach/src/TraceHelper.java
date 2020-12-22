/*import javax.servlet.ServletRequest;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpSession;*/


import com.alibaba.fastjson.JSON;
import com.asn1c.core.Bool;
import com.sun.tools.attach.AttachNotSupportedException;
import com.sun.tools.attach.VirtualMachine;
import com.sun.tools.attach.VirtualMachineDescriptor;

import java.io.IOException;
import java.text.DateFormat;
import java.util.*;

public class TraceHelper {
    private String name;
    private StringBuilder sb;
    private String $agentId(){
        return "1";
    }
    private String $pid(){
        return "1";
    }
    private String $host(){
        return "127.0.0.1";
    }
    private static native void send(String data);
    private static native void traceServlet(long t1,long t2,Object src,Object err,Object request,Object response);


    public TraceHelper(String name){
        this.name = name;
        sb = new StringBuilder().append("{\"").append(name).
                append("\":[{\"agentId\":\"").append($agentId()).append("\"").
                append(",\"pid\":\"").append($pid()).append("\"");
    }
    public void send(){
        sb.append("}]}");
        try{
            send(sb.toString());
        }catch(UnsatisfiedLinkError e){}
    }
    public TraceHelper append(String key,int value){
       sb = sb.append(",\"").append(key).append("\":").append(value);
       return this;
    }
    public TraceHelper append(String key,long value){
        sb = sb.append(",\"").append(key).append("\":").append(value);
        return this;
    }
    public TraceHelper append(String key,float value){
        sb = sb.append(",\"").append(key).append("\":").append(value);
        return this;
    }
    public TraceHelper append(String key,double value){
        sb = sb.append(",\"").append(key).append("\":").append(value);
        return this;
    }
    public TraceHelper append(String key,boolean value){
        sb = sb.append(",\"").append(key).append("\":").append(value);
        return this;
    }
    public TraceHelper append(String key,String value){
        if ("".equals(value))return this;
        sb = sb.append(",\"").append(key).append("\":\"").append(value.replaceAll("\\\\","\\\\\\\\")).append("\"");
        return this;
    }

    public String host(String host) {
        if ("0:0:0:0:0:0:0:1".equals(host)|| "127.0.0.1".equals(host)|| "localhost".equals(host)){
            return $host( );
        }else{
            return host;
        }
    }
    public static void main(String[] args) throws IOException, AttachNotSupportedException {
        List<VirtualMachineDescriptor> vms =com.sun.tools.attach.VirtualMachine.list();;
        for (VirtualMachineDescriptor vmd : vms) {
            if(vmd.displayName().equals("weblogic.Server")){
                VirtualMachine vm = VirtualMachine.attach(vmd);
                vm.getSystemProperties().forEach((k,v)->System.out.println(k +":"+v));
                vm.detach();
            }
        }
    }
}
