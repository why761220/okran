import javassist.*;
import javax.servlet.ServletRequest;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpSession;
import java.io.File;
import java.io.IOException;
import java.lang.reflect.Method;
import java.util.Date;

public class ByteCodes {

    public static void main(String[] args) {
        //modifyServletStubImpl();
    }
    private static void $trace(long ti, Exception ex, ServletRequest request) {
        HttpServletRequest req;
        long timestamp = (new Date()).getTime();
        TraceHelper sb = new TraceHelper("trace");
        long duration = (ti == 0 ? 0 : timestamp - ti);
        try {
            sb.append("timestamp",timestamp).append("duration",duration);
            if (ex != null) {
                sb.append("error",1).append("message",ex.getMessage());
            }
            if (request instanceof HttpServletRequest) {
                req = (HttpServletRequest) request;
                HttpSession session = req.getSession();
                if (session != null) {
                    Object UserView = session.getAttribute("UserView");
                    if (UserView != null) {
                        try {
                            Method m = UserView.getClass().getMethod("getDlzh");
                            Object userId = m.invoke(UserView);
                            if (userId != null) {
                                sb.append("userId",userId.toString());
                            }
                        } catch (Exception ignored) {
                        }
                    }
                }

                sb.append("uri",req.getRequestURI());

                sb.append("method",req.getMethod());

                sb.append("serverHost",sb.host(req.getLocalAddr()));

                sb.append("serverPort",req.getLocalPort());

                sb.append("remoteHost",sb.host(req.getRemoteHost()));

                sb.append("remotePort",req.getRemotePort());

                sb.append("contextPath",req.getContextPath());

                sb.append("sessionId",req.getRequestedSessionId());

                sb.append("seferer",req.getHeader("Referer"));

                sb.append("userAgent",req.getHeader("User-Agent"));
            }
            sb.send();
        } catch (Exception ignored) {

        } catch (Error ignored) {}
    }

    private static CtMethod Get$trace(ClassPool pool) throws NotFoundException {
        return pool.get(ByteCodes.class.getName()).getDeclaredMethod("$trace",
                new CtClass[]{
                        pool.get(long.class.getName()),
                        pool.get(Exception.class.getName()),
                        pool.get("javax.servlet.ServletRequest")
                });
    }

    public void ServletStubImpl(String name, String classFile,String out,String extDirs) throws NotFoundException, IOException, CannotCompileException {
        ClassPool pool = ClassPool.getDefault();
        CtMethod nm, old, cm;
        CtClass[] params;
        if (!"".equals(extDirs)){
            pool.insertClassPath(extDirs+"/lib/wlfullclient.jar");
        }
        if (classFile != null && classFile != "") {
            File file = new File(classFile);
            if (file.exists()) {
                long len = file.length();
                byte[] data = new byte[(int) len];
                java.io.FileInputStream in = new java.io.FileInputStream(file);
                int count = in.read(data);
                in.close();
                if(count == len){
                    pool.insertClassPath(new ByteArrayClassPath(name, data));
                }
            }
        }
        CtClass clazz = pool.get(name);
        nm = CtNewMethod.copy(Get$trace(pool), clazz, null);
        try {
            old = clazz.getDeclaredMethod("$trace",
                    new CtClass[]{
                            pool.get(long.class.getName()),
                            pool.get(Exception.class.getName()),
                            pool.get("javax.servlet.ServletRequest")
                    });
            clazz.removeMethod(old);
        } catch (Exception ignored) {}
        clazz.addMethod(nm);
        params = new CtClass[]{
                pool.get("javax.servlet.ServletRequest"),
                pool.get("javax.servlet.ServletResponse"),
                pool.get("weblogic.servlet.internal.FilterChainImpl")
        };
        cm = clazz.getDeclaredMethod("execute", params);
        cm.addLocalVariable("$ti", CtClass.longType);
        cm.insertBefore("$ti = (new java.util.Date()).getTime();");
        cm.insertAfter("$trace($ti,null,$1);");
        cm.addCatch("{$trace(0L,$e,$1); throw $e;}", pool.get(java.lang.Exception.class.getName()));
        clazz.writeFile(out);
    }

    public void TraceHelper(String out,String agentId,String pid,String host) throws NotFoundException, CannotCompileException, IOException {
        ClassPool pool = ClassPool.getDefault();
        CtClass clazz = pool.get(TraceHelper.class.getName());
        CtMethod m = clazz.getDeclaredMethod("$agentId",new CtClass[]{});
        m.setBody("return \"" + agentId +"\";");
        m = clazz.getDeclaredMethod("$pid",new CtClass[]{});
        m.setBody("return \"" + pid +"\";");
        m = clazz.getDeclaredMethod("$host",new CtClass[]{});
        m.setBody("return \"" + host +"\";");
        clazz.writeFile(out);
    }
}
