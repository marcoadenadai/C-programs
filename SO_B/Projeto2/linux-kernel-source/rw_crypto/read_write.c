//INTEGRANTES E RA
//Breno Baldovinotti 		| 14315311
//Caroline Gerbaudo Nakazato 	| 17164260
//Marco Antônio de Nadai Filho 	| 16245961
//Nícolas Leonardo Külzer Kupka | 16104325
//Paulo Mangabeira Birocchi 	| 16148363

#include <linux/kernel.h>
#include <linux/slab.h> 
#include <linux/stat.h>
#include <linux/fcntl.h>
#include <linux/file.h> //PARA STRUCTFD
#include <linux/uio.h>
#include <linux/fsnotify.h> //fsnotify_access
#include <linux/security.h>
#include <linux/export.h>
#include <linux/syscalls.h>
#include <linux/pagemap.h>
#include <linux/splice.h>
#include <linux/compat.h>

#include <asm/uaccess.h>
#include <asm/unistd.h>

#include <linux/scatterlist.h>
#include "crypto_ecb.h"

message * enc_dec(int mode, unsigned char * input, int size);
//-------------------------------------------------------------------------------------------------
//--FUNCOES_EM_COMUM-------------------------------------------------------------------------------
static inline struct fd fdget_pos(int fd)
{
	return __to_fd(__fdget_pos(fd));
}

static inline loff_t file_pos_read(struct file *file)
{
	return file->f_mode & FMODE_STREAM ? 0 : file->f_pos;
}
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ RW_VERIFY_AREA ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
static inline bool x_unsigned_offsets(struct file *file)
{
	return file->f_mode & FMODE_UNSIGNED_OFFSET;
}

static inline int rw_verify_area(int read_write, struct file *file, const loff_t *ppos, size_t count)
{
	struct inode *inode;
	loff_t pos;
	int retval = -EINVAL;

	inode = file_inode(file);
	if (unlikely((ssize_t) count < 0))
		return retval;
	pos = *ppos;
	if (unlikely(pos < 0)) {
		if (!x_unsigned_offsets(file))
			return retval;
		if (count >= -pos) /* both values are in 0..LLONG_MAX */
			return -EOVERFLOW;
	} else if (unlikely((loff_t) (pos + count) < 0)) {
		if (!x_unsigned_offsets(file))
			return retval;
	}

	if (unlikely(inode->i_flctx && mandatory_lock(inode))) {
		retval = locks_mandatory_area(
			read_write == READ ? FLOCK_VERIFY_READ : FLOCK_VERIFY_WRITE,
			inode, file, pos, count);
		if (retval < 0)
			return retval;
	}
	retval = security_file_permission(file,
				read_write == READ ? MAY_READ : MAY_WRITE);
	if (retval)
		return retval;
	return count > MAX_RW_COUNT ? MAX_RW_COUNT : count;
}
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

static inline void x_fdput_pos(struct fd f)
{
	if (f.flags & FDPUT_POS_UNLOCK)
		mutex_unlock(&f.file->f_pos_lock);
	fdput(f);
}

static inline void x_file_pos_write(struct file *file, loff_t pos)
{
	if ((file->f_mode & FMODE_STREAM) == 0)
		file->f_pos = pos;
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------


static inline ssize_t x_new_sync_write(struct file *filp, char *buf, size_t len, loff_t *ppos)
{
	struct iovec iov = { .iov_base = (void /*__user*/ *)buf, .iov_len = len };
	struct kiocb kiocb;
	struct iov_iter iter;
	ssize_t ret;

	init_sync_kiocb(&kiocb, filp);
	kiocb.ki_pos = *ppos;
	iov_iter_init(&iter, WRITE, &iov, 1, len);

	ret = filp->f_op->write_iter(&kiocb, &iter);
	BUG_ON(ret == -EIOCBQUEUED);
	if (ret > 0)
		*ppos = kiocb.ki_pos;
	return ret;
}
//-----------------------------------	{VFS_WRITE}		-------------------------------------------

static inline ssize_t x__vfs_write(struct file *file, char *p, size_t count, loff_t *pos)
{
	if (file->f_op->write)
		return file->f_op->write(file, p, count, pos);
	else if (file->f_op->write_iter)
		return x_new_sync_write(file, p, count, pos);
	else
		return -EINVAL;
}

static inline ssize_t vfs_write_crypt(struct file *file, char *buf, size_t count, loff_t *pos)
{
	ssize_t ret;

	if (!(file->f_mode & FMODE_WRITE))
		return -EBADF;
	if (!(file->f_mode & FMODE_CAN_WRITE))
		return -EINVAL;
	//if (unlikely(!access_ok(VERIFY_READ, buf, count)))
	//	return -EFAULT;

	ret = rw_verify_area(WRITE, file, pos, count);
	if (ret >= 0) {
		count = ret;
		file_start_write(file);
		ret = x__vfs_write(file, buf, count, pos);
		if (ret > 0) {
			fsnotify_modify(file);
			add_wchar(current, ret);
		}
		inc_syscw(current);//////////////////????????????????????????????????????
		file_end_write(file);
	}

	return ret;
}

//--------------------------------------------------------------------------------------------------

//=================================================================================================
//=====		[WRITE_CRYPT]		===================================================================

asmlinkage long write_crypt (int fd, void * buf, size_t count)
{
	pr_info("write_crypt syscall\n");

    struct fd f = fdget_pos(fd);
	ssize_t ret = -EBADF;
	int i;
	unsigned char * c = (unsigned char *)buf;
	for(i=0;i<count;i++){
		pr_info("buf[%d]=0x%X=%c\n",i,c[i],c[i]);
	}
		
//!!! WRITE NA TEORIA TA BALA
	message * M = enc_dec(ENCRYPT,c,count);
	pr_info("M->blocks = %d\n",M->blocks);
	for(i=0;i<(M->blocks*KEY_SIZE);i++){
		pr_info("M->data[%d]=0x%X=%c\n",i,(unsigned char)M->data[i],(unsigned char)M->data[i]);
	}

	//ATENCAO ERROR ACESS_OK -14 = EFAULT ACHO, BUFFER N PODE SER ACESSADOOO!!


	if (f.file) {
	    loff_t pos = file_pos_read(f.file);
	    ret = vfs_write_crypt(f.file, M->data, M->blocks*KEY_SIZE, &pos);
	if (ret >= 0)
		x_file_pos_write(f.file, pos);
	x_fdput_pos(f);
	}

	kfree(M->data);
	kfree(M);
	printk("write_crypt returned: %ld\n",ret);
	return ret;
}

//-------------------------------------------------------------------------------------------------
//=================================================================================================
//-------------------------------------------------------------------------------------------------

static inline ssize_t x_new_sync_read(struct file *filp, char *buf, size_t len, loff_t *ppos)
{
	struct iovec iov = { .iov_base = buf, .iov_len = len };
	struct kiocb kiocb;
	struct iov_iter iter;
	ssize_t ret;

	init_sync_kiocb(&kiocb, filp);
	kiocb.ki_pos = *ppos;
	iov_iter_init(&iter, READ, &iov, 1, len);

	ret = filp->f_op->read_iter(&kiocb, &iter);
	BUG_ON(ret == -EIOCBQUEUED);
	*ppos = kiocb.ki_pos;
	return ret;
}

static inline ssize_t x__vfs_read(struct file *file, char *buf, size_t count, loff_t *pos)
{
	if (file->f_op->read)
		return file->f_op->read(file, buf, count, pos);
	else if (file->f_op->read_iter)
		return x_new_sync_read(file, buf, count, pos);
	else
		return -EINVAL;
}

static inline ssize_t vfs_read_crypt(struct file *file, char *buf, size_t count, loff_t *pos)
{
	ssize_t ret;

	if (!(file->f_mode & FMODE_READ))
		return -EBADF;
	if (!(file->f_mode & FMODE_CAN_READ))
		return -EINVAL;
	if (unlikely(!access_ok(VERIFY_WRITE, buf, count)))//check if a user space pointer is valid
		return -EFAULT;
	
	ret = rw_verify_area(READ, file, pos, count);
	if (ret >= 0) {
		count = ret;
		ret = x__vfs_read(file, buf, count, pos); //aqui onde acontecem as coisas, o mecanismo
		//AQUI JA TEMOS O BUFFER CRIPTOGRAFADOOOO SO PRECISA DESCRIPTOGRAFAR PORRAAA
		
		if (ret > 0) {
			unsigned char * c = (unsigned char *)buf;
			message * M = enc_dec(DECRYPT,c,count);
			int i;
			for(i=0;i<count;i++){
				c[i]=(unsigned char)M->data[i];
			}
			kfree(M->data);
			kfree(M);
				
			fsnotify_access(file);
			add_rchar(current, ret);
		}
		inc_syscr(current);
	}

	return ret;
}

int get_padding_bytes(int raw_size, int key_size){
	int blocks = raw_size/key_size;
	if(raw_size%key_size != 0) //end_pos = raw_size%key_size
		blocks++;
	return blocks*key_size;
}

//=================================================================================================
//=====		[READ_CRYPT]		===================================================================

asmlinkage long read_crypt (int fd, void * buf, size_t count)
{
	struct fd f = fdget_pos(fd);
	ssize_t ret = -EBADF;

	//TALVEZ NAO SEJA GET_PADDING_BYTES MAS SIM DO_PADDING NO BUFF!!!!!!
	if (f.file) {
		loff_t pos = file_pos_read(f.file);
		ret = vfs_read_crypt(f.file, buf, get_padding_bytes(count,KEY_SIZE), &pos);
		if (ret >= 0)
			x_file_pos_write(f.file, pos);
		x_fdput_pos(f);
	}

	printk("read_crypt returned: %ld\n",ret);
	return ret;
} 

//=================================================================================================
//=================================================================================================

message * enc_dec(int mode, unsigned char * input, int size)
{	//ira retornar um char * e seu tamanho em blocos
    int i;
	struct skcipher_def sk;
    sk.tfm = NULL;
    sk.req = NULL;
    sk.ciphertext = NULL;
    message * INPUT = alloc_hex_padding(input,size);
	for(i=0;i<(KEY_SIZE*INPUT->blocks);i++)
		pr_info("ENTRADA[%d]=0x%X=%c\n",i,INPUT->data[i],INPUT->data[i]);

    aes_enc_dec(mode, INPUT, &sk);

    //APRESENTACAO DO RESULTADO DA ENCRIPTACAO..
    char * txt = sg_virt(&sk.out);

    for (i = 0;i<(KEY_SIZE*INPUT->blocks); i++){
		INPUT->data[i] = (unsigned char)txt[i];
		pr_info("INPUT->data[%d] = 0x%X = %c \n",i,(unsigned char)INPUT->data[i],(unsigned char)INPUT->data[i]);
	}
    
    skcipher_finish(&sk);
    return INPUT;
}
