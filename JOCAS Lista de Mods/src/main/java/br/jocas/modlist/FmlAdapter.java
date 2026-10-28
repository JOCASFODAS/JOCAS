package br.jocas.modlist;

import java.lang.reflect.*;
import java.nio.file.Path;
import java.util.*;

/** Only stable loader metadata is accessed; never Minecraft's changing/mapped classes. */
public final class FmlAdapter {
    private FmlAdapter() {}

    public static void start(String namespace, String name) {
        try {
            Object list = callStatic(namespace + ".fml.ModList", "get");
            if (list == null) throw new IllegalStateException("ModList ainda não disponível");
            Class<?> environment = Class.forName(namespace + ".fml.loading.FMLEnvironment");
            Object dist;
            try { dist = environment.getMethod("getDist").invoke(null); }
            catch (NoSuchMethodException e) { dist = environment.getField("dist").get(null); }
            boolean client = dist.toString().equals("CLIENT");
            Object gameDir = Class.forName(namespace + ".fml.loading.FMLPaths").getField("GAMEDIR").get(null);
            Path path = (Path) call(gameDir, "get");
            List<Snapshot.Entry> mods = collect((Collection<?>) call(list, "getMods"));
            String loaderId = namespace.contains("neoforged") ? "neoforge" : "forge";
            String version = mods.stream().filter(m -> m.id().equals(loaderId)).map(Snapshot.Entry::version)
                .findFirst().orElse("versão não informada");
            Export.start(Snapshot.capture(name + " " + version, client ? "cliente" : "servidor dedicado", mods), path, client);
        } catch (Exception | LinkageError e) { Export.failure(e); }
    }

    public static List<Snapshot.Entry> collect(Collection<?> metadata) throws ReflectiveOperationException {
        List<Snapshot.Entry> result = new ArrayList<>();
        for (Object mod : metadata) {
            String file = "componente interno";
            try {
                Object owner = call(mod, "getOwningFile");
                if (owner != null) file = basename(call(call(owner, "getFile"), "getFileName").toString());
            } catch (ReflectiveOperationException e) { file = "arquivo não informado"; }
            List<String> dependencies = new ArrayList<>();
            try {
                for (Object dep : (Collection<?>) call(mod, "getDependencies")) {
                    String type;
                    try { type = call(dep, "getType").toString(); }
                    catch (NoSuchMethodException e) { type = Boolean.TRUE.equals(call(dep, "isMandatory")) ? "REQUIRED" : "OPTIONAL"; }
                    dependencies.add(call(dep, "getModId") + " " + call(dep, "getVersionRange") +
                        " (" + type + ", " + call(dep, "getSide") + ")");
                }
            } catch (ReflectiveOperationException e) { dependencies.add("metadados de dependências indisponíveis"); }
            Collections.sort(dependencies);
            result.add(new Snapshot.Entry(call(mod, "getDisplayName").toString(), call(mod, "getModId").toString(),
                call(mod, "getVersion").toString(), file, String.join("; ", dependencies)));
        }
        return result;
    }

    public static String basename(String path) {
        String normalized = path.replace('\\', '/');
        return normalized.substring(normalized.lastIndexOf('/') + 1);
    }

    private static Object callStatic(String name, String method) throws ReflectiveOperationException {
        return Class.forName(name).getMethod(method).invoke(null);
    }

    public static Object call(Object target, String name) throws ReflectiveOperationException {
        Method method = target.getClass().getMethod(name);
        if (!method.canAccess(target)) method = publicMethod(target.getClass(), name);
        return method.invoke(target);
    }

    private static Method publicMethod(Class<?> type, String name) throws NoSuchMethodException {
        for (Class<?> iface : type.getInterfaces()) {
            try { return iface.getMethod(name); } catch (NoSuchMethodException ignored) { }
        }
        Class<?> parent = type.getSuperclass();
        if (parent != null) {
            if (Modifier.isPublic(parent.getModifiers())) {
                try { return parent.getMethod(name); } catch (NoSuchMethodException ignored) { }
            }
            return publicMethod(parent, name);
        }
        throw new NoSuchMethodException(name);
    }
}
