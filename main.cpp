#include <filesystem>
#include <iostream>
#include <vector>

int main(int argc, const char **argv)
{
    if (argc < 2)
        return -1;

    const auto input = std::filesystem::path{argv[1]};
    if (!std::filesystem::exists(input)) {
        std::cerr << "Can't open " << argv[1] << std::endl;
        return -1;
    }

    std::filesystem::path output = input;
    if (argc == 2) {
        if (output.extension() != ".img")
            output.replace_extension(".img");
        else
            output.replace_extension(".raw");
    } else {
        output = argv[2];
    }
    auto fi = fopen(input.c_str(), "rb");
    if (!fi) {
        std::cerr << "Can't open " << input.c_str() << std::endl;
        return -1;
    }
    auto fo = fopen(output.c_str(), "wb");
    if (!fo) {
        fclose(fi);
        std::cerr << "Can't open " << output.c_str() << std::endl;
        return -1;
    }

    std::vector<uint8_t> buff(0x100);
    auto sz = fread(buff.data(), 1, 0x100, fi);
    if (sz != 0x100) {
        fclose(fi);
        fclose(fo);
        std::cerr << "Invalid file size " << input.c_str() << std::endl;
        return -1;
    }
    if (std::string{reinterpret_cast<const char*>(buff.data()), 8} != "MV - CPC") {
        fclose(fi);
        fclose(fo);
        std::cerr << "Invalid file header " << input.c_str() << std::endl;
        return -1;
    }
    auto tracks = buff[0x30];
    auto heads = buff[0x31];
    auto sztrk = *reinterpret_cast<uint16_t*>(&buff[0x32]);
    buff.resize(sztrk);
    while (heads--) {
        auto tks = tracks;
        while(tks--) {
            sz = fread(buff.data(), 1, sztrk, fi);
            if (sz != sztrk) {
                fclose(fi);
                fclose(fo);
                std::cerr << "Can't read from " << input.c_str() << std::endl;
                return -1;
            }
            int track  = buff[0x10];
            int head = buff[0x11];
            int bps = buff[0x14];
            int spt = buff[0x15];
            std::cout << "Track " << track  << ", head " << head << ", bps " << 0x100 * bps << ",spt " << spt << std::endl;
            sz = fwrite(&buff[0x100], 1, sz - 0x100, fo);
            if (sz != size_t(sztrk - 0x100)) {
                fclose(fi);
                fclose(fo);
                std::cerr << "Can't write to " << output.c_str() << std::endl;
                return -1;
            }
        }
    }
    fclose(fi);
    fclose(fo);
    return 0;
}
