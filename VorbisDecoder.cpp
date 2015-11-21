#include "Config.hpp"
#ifdef ENABLE_VORBIS

#include "VorbisDecoder.hpp"


using namespace std;

using sample_t = IDecoder::sample_t;
using byte_t   = IDecoder::byte_t;


// �R���X�g���N�^
VorbisDecoder::VorbisDecoder(unique_ptr<istream> p_input, int p_bitNum) : input(move(p_input)) {
	// �o�b�t�@�^�C�v����
	switch (p_bitNum) {
		case 32:
			bufferType = BUFFERTYPE::BUFFERTYPE_FLOAT;
			break;

		case 16:
			bufferType = BUFFERTYPE::BUFFERTYPE_INT16;
			break;

		case 8:
			bufferType = BUFFERTYPE::BUFFERTYPE_INT8;
			break;

		default:
			throw ERRORCODE_INVALID_ARGUMENT;
	}

	// ��O�ݒ�
	input->exceptions(istream::failbit | istream::badbit);

	// Ogg Vorbis���J��
	auto ret = ov_open_callbacks(this, &vf, nullptr, 0, {read, seek, close, tell});
	if (ret != 0) {
		// �ǂݍ��߂Ȃ�����
		throw ERRORCODE_INVALID_FORMAT;
	}

	// ���T���v�����擾
	bufferLength = ov_pcm_total(&vf, -1);

	// �`�����l�����ƃT���v�����O���[�g���擾
	vorbis_info *vi = ov_info(&vf, -1);
	channelNum = vi->channels;
	samplingRate = vi->rate;
}

// �f�X�g���N�^
VorbisDecoder::~VorbisDecoder() {
	// Ogg Vorbis�̌�Еt��
	ov_clear(&vf);
}


// �ȉ��C���^�[�t�F�C�X
sample_t VorbisDecoder::_Tell() {
	ogg_int64_t position = ov_pcm_tell(&vf);
	if (position < 0) throw position;
	return position;
}

void VorbisDecoder::_Seek(sample_t position) {
	auto ret = ov_pcm_seek(&vf, position);
	if (ret < 0) throw ret;
}

sample_t VorbisDecoder::_Read(void *ptr, sample_t length) {
	switch (bufferType) {
		// Float�^
		case BUFFERTYPE::BUFFERTYPE_FLOAT:
		{
			int currentSection;
			float **buffer;

			long read = ov_read_float(&vf, &buffer, length, &currentSection);
			if (read < 0) throw read;

			// buffer[�`�����l��][�ʒu]�ɂ���f�[�^��ptr�ɏ����Ă���
			// ���я��I��memcpy�͎g���Ȃ����H
			// �p�t�H�[�}���X������
			float *ret = (float *)ptr;
			for (sample_t i = 0; i < read; i++) {
				for (int j = 0; j < channelNum; j++) {
					*ret = buffer[j][i];
					ret++;
				}
			}
			return read;
		}

		// �����^
		case BUFFERTYPE::BUFFERTYPE_INT8:
		case BUFFERTYPE::BUFFERTYPE_INT8U:
		case BUFFERTYPE::BUFFERTYPE_INT16:
		case BUFFERTYPE::BUFFERTYPE_INT16U:
		{
			int currentSection;
			long read = ov_read(
				&vf,
				(char *)ptr,
				SampleToByte(length),
				0,
				bufferType == BUFFERTYPE::BUFFERTYPE_INT16 || bufferType == BUFFERTYPE::BUFFERTYPE_INT16U ? 2 : 1,
				bufferType == BUFFERTYPE::BUFFERTYPE_INT16 || bufferType == BUFFERTYPE::BUFFERTYPE_INT8   ? 1 : 0,
				&currentSection
			);
			if (read < 0) throw read;
			return ByteToSample(read);
		}
	}
	// Unreachable code
	throw ERRORCODE_INTERNAL_ERROR;
}


// callbacks
int VorbisDecoder::close(void *datasource) {
	if (datasource == nullptr) return -1;
	auto decoder = (VorbisDecoder *)datasource;
	if (!decoder->input) return -2;
	try {
		decoder->input.reset();
		return 0;
	} catch (...) {
		return -3;
	}
}

size_t VorbisDecoder::read(void *ptr, size_t size, size_t nmemb, void *datasource) {
	if (datasource == nullptr) return 0;
	auto decoder = (VorbisDecoder *)datasource;
	if (!decoder->input) return 0;
	try {
		decoder->input->read((char *)ptr, size * nmemb);
		return decoder->input->gcount();
	} catch (...) {
		return 0;
	}
}

int VorbisDecoder::seek(void *datasource, ogg_int64_t offset, int whence) {
	if (datasource == nullptr) return -1;
	auto decoder = (VorbisDecoder *)datasource;
	if (!decoder->input) return -1;
	try {
		ios_base::seekdir way;
		switch (whence) {
			case SEEK_SET: way = ios_base::beg; break;
			case SEEK_CUR: way = ios_base::cur; break;
			case SEEK_END: way = ios_base::end; break;
			default: throw 1;
		}
		if (decoder->input->eof() && decoder->input->fail()) decoder->input->clear();
		decoder->input->seekg(offset, way);
		return 0;
	} catch (...) {
		return -1;
	}
}

long VorbisDecoder::tell(void *datasource) {
	if (datasource == nullptr) return -1;
	auto decoder = (VorbisDecoder *)datasource;
	if (!decoder->input) return -1;
	try {
		return decoder->input->tellg();
	} catch (...) {
		return -1;
	}
}

#endif