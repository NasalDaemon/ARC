"""Macro to generate embedded modules from .cpp files."""

def gen_embed(name, src, out_name, venv_python, generator):
    """Generate embedded module from .cpp file.

    Args:
        name: Target suffix name
        src: Source .cpp file
        out_name: Output file name
        venv_python: Path to venv python
        generator: Path to generator script
    """
    native.genrule(
        name = "gen_embed_" + name,
        srcs = [src],
        outs = ["gen/embed_" + out_name],
        cmd = "{venv} {gen} -q -m -i $(location {src}) -o $@".format(
            venv = venv_python,
            gen = generator,
            src = src,
        ),
    )
