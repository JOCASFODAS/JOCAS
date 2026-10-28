package br.jocas.modlist;

import java.lang.reflect.*;
import java.util.*;

/** Contract test against actual loader JARs, using proxies for game metadata. */
public class RealFmlApi {
    static Object proxy(Class<?> type, Map<String, Object> methods) {
        return Proxy.newProxyInstance(type.getClassLoader(), new Class[]{type}, (p, m, a) -> methods.get(m.getName()));
    }
    public static void main(String[] args) throws Exception {
        String prefix = args.length > 0 ? args[0] : "net.neoforged";
        String spi = prefix.equals("net.neoforged") ? ".neoforgespi" : ".forgespi";
        Class<?> info = Class.forName(prefix + spi + ".language.IModInfo");
        Class<?> dep = Class.forName(info.getName() + "$ModVersion");
        Object version = Class.forName("org.apache.maven.artifact.versioning.DefaultArtifactVersion").getConstructor(String.class).newInstance("1.2.3");
        Object range = Class.forName("org.apache.maven.artifact.versioning.VersionRange").getMethod("createFromVersionSpec", String.class).invoke(null, "[1.20.1,)");
        Map<String, Object> values = new HashMap<>();
        values.put("getModId", "minecraft"); values.put("getVersionRange", range);
        Class<?> side = dep.getMethod("getSide").getReturnType();
        values.put("getSide", Arrays.stream(side.getEnumConstants()).filter(x -> x.toString().equals("BOTH")).findFirst().orElseThrow());
        try {
            Class<?> type = dep.getMethod("getType").getReturnType();
            values.put("getType", Arrays.stream(type.getEnumConstants()).filter(x -> x.toString().equals("REQUIRED")).findFirst().orElseThrow());
        } catch (NoSuchMethodException e) { dep.getMethod("isMandatory"); values.put("isMandatory", true); }
        Map<String, Object> metadata = new HashMap<>();
        metadata.put("getDisplayName", "Contrato real"); metadata.put("getModId", "api_test"); metadata.put("getVersion", version);
        metadata.put("getDependencies", List.of(proxy(dep, values)));
        var result = FmlAdapter.collect(List.of(proxy(info, metadata)));
        if (!result.get(0).dependencies().contains("REQUIRED") || !result.get(0).version().equals("1.2.3")) throw new AssertionError(result);
        Class<?> list = Class.forName(prefix + ".fml.ModList", false, info.getClassLoader());
        list.getMethod("get"); list.getMethod("getMods");
        Class<?> env = Class.forName(prefix + ".fml.loading.FMLEnvironment", false, info.getClassLoader());
        try { env.getMethod("getDist"); } catch (NoSuchMethodException e) { env.getField("dist"); }
        Class<?> paths = Class.forName(prefix + ".fml.loading.FMLPaths", false, info.getClassLoader());
        paths.getField("GAMEDIR"); paths.getMethod("get");
        System.out.println("OK: API real " + info.getProtectionDomain().getCodeSource().getLocation() + " — coleta, dependências e assinaturas de inicialização.");
    }
}
