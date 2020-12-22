
import com.alibaba.fastjson.JSON;
import com.sun.tools.attach.*;
import javassist.CannotCompileException;
import javassist.NotFoundException;

import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Properties;

public class Attach {
    private static String version() {
        return "1.0.0";
    }

    private static String listJavaProcesses(String name) {

        String self = Attach.class.getProtectionDomain().getCodeSource().getLocation().getFile();
        if (self.length()>1 && self.charAt(0) == '/'){
            self = self.substring(1);
        }
        List<VirtualMachineDescriptor> vms;
        List<HashMap<String, Object>> out = new ArrayList<>();
        vms = com.sun.tools.attach.VirtualMachine.list();
        for (int i = 0; i < vms.size(); i++) {
            VirtualMachineDescriptor vmd = vms.get(i);
            HashMap<String, Object> ret = new HashMap<String, Object>();
            if (vmd.displayName().replaceAll("\\\\","/").contains(self)){
                continue;
            }else if("".equals(name) || "*".equals(name) || vmd.displayName().equals(name)) {
                ret.put("name", vmd.displayName());
                ret.put("pid", vmd.id());
                try {
                    VirtualMachine vm = com.sun.tools.attach.VirtualMachine.attach(vmd);
                    Properties ps = vm.getSystemProperties();
                    ret.put("wls.home", ps.getProperty("wls.home",ps.getProperty("weblogic.home","")));
                    vm.detach();
                } catch (AttachNotSupportedException e) {
                    e.printStackTrace();
                } catch (IOException e) {
                    e.printStackTrace();
                }
                out.add(ret);
            }
        }
        return JSON.toJSONString(out);
    }

    public static String attach(String[] args) throws IOException, AttachNotSupportedException, AgentLoadException, AgentInitializationException {
        if (args.length < 4) {
            return errForParams();
        }
        String pid = args[1];
        String agentFile = args[2];
        String attachArgs = args[3];
        List<VirtualMachineDescriptor> vms;
        vms = com.sun.tools.attach.VirtualMachine.list();
        HashMap<String, Object> ret = new HashMap<>();

            for (VirtualMachineDescriptor vmd : vms) {
                if (vmd.id().equals(pid)) {
                    VirtualMachine vm = VirtualMachine.attach(vmd);
                    Properties ps = vm.getSystemProperties();
                    String wlsHome = ps.getProperty("wls.home",ps.getProperty("weblogic.home",""));
                    if (!"".equals(wlsHome)){
                        attachArgs += ";wls.home=" + wlsHome;
                        ret.put("wls.home",wlsHome);
                    }
                    vm.loadAgentPath(agentFile, attachArgs);
                    vm.detach();
                    ret.put("pid",vmd.id());
                    ret.put("displayName",vmd.displayName());

                }
            }

        return JSON.toJSONString(ret);
    }

    private static String errForParams() {
        return "{\"error\":\"params is error!\"}";
    }

    private static String DefineClasses(String[] args) {
        HashMap<String, Object> ret = new HashMap<>();
        ArrayList<String> list = new ArrayList<>();
        list.add(TraceHelper.class.getName());
        ret.put("classes", list);
        return JSON.toJSONString(ret);
    }

    private static String RedefineClasses(String[] args) {
        HashMap<String, Object> ret = new HashMap<>();
        ArrayList<String> list = new ArrayList<>();
        list.add("weblogic.servlet.internal.ServletStubImpl");
        ret.put("classes", list);
        return JSON.toJSONString(ret);
    }

    private static String bytecode(String args[]) throws IOException, CannotCompileException, NotFoundException {
        if (args.length < 2) {
            return errForParams();
        }
        HashMap<String, Object> ret = new HashMap<>();
        switch (args[1]) {
            case "weblogic.servlet.internal.ServletStubImpl":
                (new ByteCodes()).ServletStubImpl(args[1], args[2], args[3],args[4]);
                break;
            case "TraceHelper":
                if(args.length>6){
                    (new ByteCodes()).TraceHelper(args[3],args[4],args[5],args[6]);
                }else{
                    return errForParams();
                }
                break;
            default:
                return errForParams();
        }
        return JSON.toJSONString(ret);
    }

    public static void main(String[] args) throws NotFoundException, CannotCompileException, IOException, AttachNotSupportedException, AgentLoadException, AgentInitializationException {

        if (args.length == 0) {
            return;
        }
        switch (args[0]) {
            case "DefineClasses":
                System.out.println(DefineClasses(args));
                return;
            case "RedefineClasses":
                System.out.println(RedefineClasses(args));
                return;
            case "bytecode":
                System.out.println(bytecode(args));
                return;
            case "attach":
                System.out.println(attach(args));
                break;
            case "list":
                String s = listJavaProcesses( args.length > 1 ? args[1] : "");
                System.out.println(s);
                break;
            default:
                HashMap<String, String> r = new HashMap<>();
                Properties props = System.getProperties();
                for (Object k : props.keySet()) {
                    r.put(k.toString(), props.getProperty(k.toString()));
                }
                r.put("okran.version", version());
                System.out.println(JSON.toJSONString(r));
                break;
        }
    }
}
