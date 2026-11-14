package com.cowlark.fluxengine.buildutils;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.config.OptionGroupProto;
import com.cowlark.fluxengine.config.OptionProto;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.Message;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import java.util.Map;
import org.protowire.pxf.Pxf;

/* Build-time validation of option defaults in PXF configurations. */
public final class LintPxf
{
    private LintPxf()
    {
    }

    public static void main(String[] args) throws Exception
    {
        /* Check every input before failing so all missing defaults are reported together. */
        boolean valid = true;
        for (String filename : args)
        {
            ConfigProto.Builder builder = ConfigProto.newBuilder();
            try
            {
                Pxf.unmarshal(Files.readAllBytes(Path.of(filename)), builder);
                valid &= check(builder.build(), filename);
            } catch (Exception e)
            {
                System.err.printf("%s: cannot validate PXF: %s%n", filename, e.getMessage());
                valid = false;
            }
        }
        if (!valid)
            System.exit(1);
    }

    private static boolean check(Message message, String location)
    {
        /* Option groups require an explicit default even when no preferences are saved. */
        boolean valid = true;
        if (message instanceof OptionGroupProto group)
        {
            boolean hasDefault = false;
            for (OptionProto option : group.getOptionList())
                hasDefault |= option.getSetByDefault();
            if (!hasDefault)
            {
                System.err.printf("%s: option group '%s' (%s) has no set_by_default = true%n",
                        location, group.getName(), group.getComment());
                valid = false;
            }
        }

        /* Traverse nested messages too, including configurations embedded in options. */
        for (Map.Entry<FieldDescriptor, Object> entry : message.getAllFields().entrySet())
        {
            FieldDescriptor field = entry.getKey();
            if (field.getJavaType() != FieldDescriptor.JavaType.MESSAGE)
                continue;
            String childLocation = location + "." + field.getName();
            if (field.isRepeated())
            {
                List<?> children = (List<?>) entry.getValue();
                for (int index = 0; index < children.size(); index++)
                    valid &= check((Message) children.get(index), childLocation + "[" + index + "]");
            }
            else
                valid &= check((Message) entry.getValue(), childLocation);
        }
        return valid;
    }
}
