package com.cowlark.fluxengine.config;

/**
 * Thrown when a config path cannot be resolved against a protobuf, ported
 * from lib/config/proto.h.
 */
public class ProtoPathNotFoundException extends ConfigException
{
    public ProtoPathNotFoundException(String message)
    {
        super(message);
    }
}
